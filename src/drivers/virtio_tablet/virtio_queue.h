#ifndef VIRTQUEUE_H
#define VIRTQUEUE_H
/* An interface for efficient virtio implementation.
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers.
 *
 * Copyright 2007, 2009, IBM Corporation
 * Copyright 2011, Red Hat, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define uint64_t UINT64
#define uint32_t UINT32
#define uint16_t UINT16
#define int16_t INT16
#define int32_t INT32
#define uint8_t UINT8

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT       1
/* This marks a buffer as write-only (otherwise read-only). */
#define VIRTQ_DESC_F_WRITE      2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT   4

/* The device uses this in used->flags to advise the driver: don't kick me
 * when you add a buffer.  It's unreliable, so it's simply an
 * optimization. */
#define VIRTQ_USED_F_NO_NOTIFY  1
/* The driver uses this in avail->flags to advise the device: don't
 * interrupt me when you consume a buffer.  It's unreliable, so it's
 * simply an optimization.  */
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1

/* Support for indirect descriptors */
#define VIRTIO_F_INDIRECT_DESC    (1 << 28)

/* Support for avail_event and used_event fields */
#define VIRTIO_F_EVENT_IDX        (1 << 29)

/* Arbitrary descriptor layouts. */
#define VIRTIO_F_ANY_LAYOUT       (1 << 27)

#pragma pack (push, 1)

/* Virtqueue descriptors: 16 bytes.
 * These can chain together via "next". */
struct virtq_desc {
        /* Address (guest-physical). */
        uint64_t addr;
        /* Length. */
        uint32_t len;
        /* The flags as indicated above. */
        uint16_t flags;
        /* We chain unused descriptors via this, too */
        uint16_t next;
};

struct virtq_avail {
        uint16_t flags;
        uint16_t idx;
        /* uint16_t ring[]; */
        /* Only if VIRTIO_F_EVENT_IDX: uint16_t used_event; */
};

/* uint32_t is used here for ids for padding reasons. */
struct virtq_used_elem {
        /* Index of start of used descriptor chain. */
        uint32_t id;
        /* Total length of the descriptor chain which was written to. */
        uint32_t len;
};

struct virtq_used {
        uint16_t flags;
        uint16_t idx;
        /* struct virtq_used_elem ring[]; */
        /* Only if VIRTIO_F_EVENT_IDX: uint16_t avail_event; */
}; 

struct virtio_input_event { 
  int16_t type; 
  int16_t code; 
  int32_t value; 
}; 

#pragma pack (pop)

enum virtio_input_config_select { 
  VIRTIO_INPUT_CFG_UNSET      = 0x00, 
  VIRTIO_INPUT_CFG_ID_NAME    = 0x01, 
  VIRTIO_INPUT_CFG_ID_SERIAL  = 0x02, 
  VIRTIO_INPUT_CFG_ID_DEVIDS  = 0x03, 
  VIRTIO_INPUT_CFG_PROP_BITS  = 0x10, 
  VIRTIO_INPUT_CFG_EV_BITS    = 0x11, 
  VIRTIO_INPUT_CFG_ABS_INFO   = 0x12, 
};

struct virtq {
        uint32_t num;
		uint32_t num_mask;

        struct virtq_desc *desc;
        struct virtq_avail *avail;
        struct virtq_used *used;

		struct virtio_input_event *events;

		void *virt_addr;
		uint32_t meta_phys_addr;
		uint32_t last_used_idx;
		uint32_t next_fill_pos;
};

#define VIRTQ_NEED_EVENT(event_idx, new_idx, old_idx) ((uint16_t)(new_idx - event_idx - 1) < (uint16_t)(new_idx - old_idx))

#endif /* VIRTQUEUE_H */
