/*
 * bms_can.cpp
 *
 *  Created on: Dec 5, 2018
 *      Author: @fedefiorini
 */

#include "bms_can.hpp"

#include "protocol.hpp"
#include "ecu.hpp"
/*
 * The entire implementation of the BMS CAN section is taken
 * directly from DUT17 code, mainly because it has been proved
 * to work and it provides reliability in the transmission
 *
 * Original author: @BernardBekker
 */

/* Reserve some bytes for the on_chip CAN driver */
#define __SECTION(type, bank) __attribute__((section("." #type ".$" #bank)))
#define __BSS(bank) __SECTION(bss, bank)
__BSS(RESERVED) char CAN_driver_memory[256];

namespace
{
	/*
	 * This class represents a buffer of CAN frames.
	 * Parameter SIZE represents the amout of frames that can fit into the buffer
	 */
	template<unsigned int SIZE>
	class Buffer
	{
		/*
		 * Read_Pointer is the index of the next entry to be read in the Buffer
		 * Write_Pointer is the index of the next entry to write to in the Buffer
		 */
		uint8_t read_pointer = 0;
		uint8_t write_pointer = 0;

		CCAN_MSG_OBJ buffer[SIZE];

	public:
		/*
		 * Returns whether the buffer is full
		 */
		inline bool is_full()
		{
			return (write_pointer + 1) % SIZE == read_pointer;
		}

		/*
		 * Returns whether the buffer is empty
		 */
		inline bool is_empty()
		{
			return write_pointer == read_pointer;
		}

		/*
		 * Get the next element to be filled in the buffer.
		 * When data is retrieved, use push() to get the message into the buffer
		 */
		inline CCAN_MSG_OBJ *get_front()
		{
			return &buffer[write_pointer];
		}

		/*
		 * Get the last element of the buffer.
		 * When data is read, use pop() to delete this message
		 */
		inline const CCAN_MSG_OBJ *get_back()
		{
			return &buffer[read_pointer];
		}

		/*
		 * Push next element into the buffer
		 */
		inline void push()
		{
			write_pointer = (write_pointer + 1) % SIZE;
		}

		/*
		 * Remove last element from the buffer
		 */
		inline void pop()
		{
			read_pointer = (read_pointer + 1) % SIZE;
		}
	};

	Buffer<50> buffer;

	/*
	 * Callback function called by the ISR() upon message reception
	 */
	void CAN_rx(uint8_t msg_obj_num)
	{
		/* Receive only if the new message fits into the buffer */
		if (!buffer.is_full() && msg_obj_num == 1)
		{
			buffer.get_front()->msgobj = msg_obj_num;
			LPC_CCAN_API->can_receive(buffer.get_front());
			buffer.push();
		}
		else
		{
			CCAN_MSG_OBJ tmp;

			tmp.msgobj = msg_obj_num;
			LPC_CCAN_API->can_receive(&tmp);
		}
	}

	/*
	 * This parameter is used to communicate between the transmit callback function
	 * and the transmit method.
	 * If a new message is transmitted without waiting for the previous one to be actually
	 * sent, the message wouldn't be transmitted.
	 */
	volatile bool transmitting = false;

	/*
	 * Callback function called by the ISR() upon message transmission
	 */
	void CAN_tx(__attribute__ ((unused)) uint8_t msg_obj_num)
	{
		transmitting = false;
	}

	/*
	 * Callback function called by the ISR() when an error has occurred
	 */
	void CAN_error(uint32_t error_num)
	{
		if (error_num & CAN_ERROR_BOFF)
		{
			/* Resets the CAN bus */
			LPC_CAN->CNTL &= ~1;
			transmitting = false;
		}
	}
}

/*
 * Interrupt handler for the CAN driver. Just calls the already defined ISR() procedure.
 * extern "C" notation is required to let the compiler know that this handler matches
 * the weak symbol defined by the CMSIS
 */
extern "C" __attribute__ ((interrupt)) void CAN_IRQHandler(void)
{
	LPC_CCAN_API->isr();
}

namespace can
{
	void init_can()
	{
		/* Initialize the CAN peripheral */
		uint32_t can_init_settings[2] =
		{
				/*
				 * 						1Mbps			500Kbps
				 * 	CANCLKDIV			0x00000001UL	0x00000003UL
				 * 	CAN_BTR				0x00002302UL	0x00002302UL
				 */
				0x00000001UL,
				0x00002302UL
		};

		LPC_CCAN_API->init_can(&can_init_settings[0], 1);

		/* Configure the callbacks */
		CCAN_CALLBACKS callbacks =
		{
				CAN_rx,
				CAN_tx,
				CAN_error,
				0,
				0,
				0,
				0,
				0
		};
		LPC_CCAN_API->config_calb(&callbacks);

		/* Enables CAN interrupt handler */
		NVIC_EnableIRQ(CAN_IRQn);

		/* FIXME: is it correct? */
		protocol::can_id id1(protocol::ECU, 0, protocol::ANNOUNCE_ONLINE);

		CCAN_MSG_OBJ msg_obj;
		msg_obj.msgobj = 1;
		msg_obj.mode_id = id1.get_value();
		msg_obj.mask = 0x7E0;

		LPC_CCAN_API->config_rxmsgobj(&msg_obj);

		/* Enables CAN power-up on PCB */
		gpio::set(pin::CAN_EN);
	}

	void send(message *msg)
	{
		volatile uint32_t count = 0;

		/* Busy waits upon completion of previous message transmission */
		while (transmitting && count++ < 0x0A00);

		CCAN_MSG_OBJ msg_obj;
		msg_obj.msgobj = 2;
		msg_obj.mode_id = msg->id;
		msg_obj.mask = 0x0;
		msg_obj.dlc = msg->length;
		for (uint8_t i = 0; i < msg->length; i++) msg_obj.data[i] = msg->data[i];

		transmitting = true;

		LPC_CCAN_API->can_transmit(&msg_obj);
	}

	bool receive(message *msg)
	{
		if (buffer.is_empty()) return false;	/* No message received */
		else
		{
			msg->id = buffer.get_back()->mode_id;
			msg->length = buffer.get_back()->dlc;
			for (uint8_t i = 0; i < buffer.get_back()->dlc; i++) msg->data[i] = buffer.get_back()->data[i];

			buffer.pop();

			return true;
		}
	}
}

namespace communication
{
	void send_data(protocol::CanPackage *can_package)
	{
		/* Retrieve CAN ID from can_package */
		protocol::can_id can_id(protocol::device_type::BMS, 0, (protocol::package_type) can_package->pckg_t);

		/* Construct CAN message to be sent and send it through the bus */
		can::message can_message;
		can_message.id = can_id.get_value();
		can_message.length = can_package->size;
		for (int i=0; i<can_package->size; i++)
		{
			can_message.data[i] = *(((uint8_t *) can_package) + sizeof(protocol::CanPackage) + i);
		}

		can::send(&can_message);
	}
}
