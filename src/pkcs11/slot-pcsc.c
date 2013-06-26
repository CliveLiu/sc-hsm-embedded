/**
 * SmartCard-HSM PKCS#11 Module
 *
 * Copyright (c) 2013, CardContact Systems GmbH, Minden, Germany
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of CardContact Systems GmbH nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CardContact Systems GmbH BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file    slot-pcsc.c
 * @author  Frank Thater
 * @brief   Slot implementation for PC/SC reader
 */

#ifndef CTAPI

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <pkcs11/slot.h>
#include <pkcs11/token.h>
#include <pkcs11/slotpool.h>
#include <pkcs11/slot-pcsc.h>

#include <strbpcpy.h>

#ifdef DEBUG
#include <pkcs11/debug.h>
#endif

#ifndef _WIN32
#include <pcsclite.h>
#endif

#include <winscard.h>

extern struct p11Context_t *context;

static SCARDCONTEXT hContext = 0;

#ifdef DEBUG

char* pcsc_error_to_string(const DWORD error) {
	static char strError[75];

	switch (error) {
		case SCARD_S_SUCCESS :
			(void) strncpy(strError, "Command successful.", sizeof(strError));
			break;
		case SCARD_F_INTERNAL_ERROR :
			(void) strncpy(strError, "Internal error.", sizeof(strError));
			break;
		case SCARD_E_CANCELLED :
			(void) strncpy(strError, "Command cancelled.", sizeof(strError));
			break;
		case SCARD_E_INVALID_HANDLE :
			(void) strncpy(strError, "Invalid handle.", sizeof(strError));
			break;
		case SCARD_E_INVALID_PARAMETER :
			(void) strncpy(strError, "Invalid parameter given.", sizeof(strError));
			break;
		case SCARD_E_INVALID_TARGET :
			(void) strncpy(strError, "Invalid target given.", sizeof(strError));
			break;
		case SCARD_E_NO_MEMORY :
			(void) strncpy(strError, "Not enough memory.", sizeof(strError));
			break;
		case SCARD_F_WAITED_TOO_LONG :
			(void) strncpy(strError, "Waited too long.", sizeof(strError));
			break;
		case SCARD_E_INSUFFICIENT_BUFFER :
			(void) strncpy(strError, "Insufficient buffer.", sizeof(strError));
			break;
		case SCARD_E_UNKNOWN_READER :
			(void) strncpy(strError, "Unknown reader specified.", sizeof(strError));
			break;
		case SCARD_E_TIMEOUT :
			(void) strncpy(strError, "Command timeout.", sizeof(strError));
			break;
		case SCARD_E_SHARING_VIOLATION :
			(void) strncpy(strError, "Sharing violation.", sizeof(strError));
			break;
		case SCARD_E_NO_SMARTCARD :
			(void) strncpy(strError, "No smart card inserted.", sizeof(strError));
			break;
		case SCARD_E_UNKNOWN_CARD :
			(void) strncpy(strError, "Unknown card.", sizeof(strError));
			break;
		case SCARD_E_CANT_DISPOSE :
			(void) strncpy(strError, "Cannot dispose handle.", sizeof(strError));
			break;
		case SCARD_E_PROTO_MISMATCH :
			(void) strncpy(strError, "Card protocol mismatch.", sizeof(strError));
			break;
		case SCARD_E_NOT_READY :
			(void) strncpy(strError, "Subsystem not ready.", sizeof(strError));
			break;
		case SCARD_E_INVALID_VALUE :
			(void) strncpy(strError, "Invalid value given.", sizeof(strError));
			break;
		case SCARD_E_SYSTEM_CANCELLED :
			(void) strncpy(strError, "System cancelled.", sizeof(strError));
			break;
		case SCARD_F_COMM_ERROR :
			(void) strncpy(strError, "RPC transport error.", sizeof(strError));
			break;
		case SCARD_F_UNKNOWN_ERROR :
			(void) strncpy(strError, "Unknown error.", sizeof(strError));
			break;
		case SCARD_E_INVALID_ATR :
			(void) strncpy(strError, "Invalid ATR.", sizeof(strError));
			break;
		case SCARD_E_NOT_TRANSACTED :
			(void) strncpy(strError, "Transaction failed.", sizeof(strError));
			break;
		case SCARD_E_READER_UNAVAILABLE :
			(void) strncpy(strError, "Reader is unavailable.", sizeof(strError));
			break;
		case SCARD_E_PCI_TOO_SMALL :
			(void) strncpy(strError, "PCI struct too small.", sizeof(strError));
			break;
		case SCARD_E_READER_UNSUPPORTED :
			(void) strncpy(strError, "Reader is unsupported.", sizeof(strError));
			break;
		case SCARD_E_DUPLICATE_READER :
			(void) strncpy(strError, "Reader already exists.", sizeof(strError));
			break;
		case SCARD_E_CARD_UNSUPPORTED :
			(void) strncpy(strError, "Card is unsupported.", sizeof(strError));
			break;
		case SCARD_E_NO_SERVICE :
			(void) strncpy(strError, "Service not available.", sizeof(strError));
			break;
		case SCARD_E_SERVICE_STOPPED :
			(void) strncpy(strError, "Service was stopped.", sizeof(strError));
			break;
		case SCARD_E_NO_READERS_AVAILABLE :
			(void) strncpy(strError, "Cannot find a smart card reader.",
					sizeof(strError));
			break;
		case SCARD_W_UNSUPPORTED_CARD :
			(void) strncpy(strError, "Card is not supported.", sizeof(strError));
			break;
		case SCARD_W_UNRESPONSIVE_CARD :
			(void) strncpy(strError, "Card is unresponsive.", sizeof(strError));
			break;
		case SCARD_W_UNPOWERED_CARD :
			(void) strncpy(strError, "Card is unpowered.", sizeof(strError));
			break;
		case SCARD_W_RESET_CARD :
			(void) strncpy(strError, "Card was reset.", sizeof(strError));
			break;
		case SCARD_W_REMOVED_CARD :
			(void) strncpy(strError, "Card was removed.", sizeof(strError));
			break;
		case SCARD_E_UNSUPPORTED_FEATURE :
			(void) strncpy(strError, "Feature not supported.", sizeof(strError));
			break;		
	};

	/* add a null byte */
	strError[sizeof(strError) - 1] = '\0';

	return strError;
}

#endif



/**
 * Transmit APDU using PC/SC
 *
 * @param slot the slot to use for communication
 * @param capdu the command APDU
 * @param capdu_len the length of the command APDU
 * @param rapdu the response APDU
 * @param rapdu_len the length of the response APDU
 * @return -1 for error or length of received response APDU
 */
int transmitAPDUviaPCSC(struct p11Slot_t *slot,
	unsigned char *capdu, size_t capdu_len,
	unsigned char *rapdu, size_t rapdu_len)
{
	int rc;
	DWORD lenr;

	FUNC_CALLED();

	if (!slot->card) {
		FUNC_FAILS(CKR_DEVICE_ERROR, "No card handle");
	}

	lenr = rapdu_len;

	rc = SCardTransmit(slot->card, SCARD_PCI_T1, capdu, capdu_len, NULL, rapdu, &lenr);

	if (rc != SCARD_S_SUCCESS) {
#ifdef DEBUG
		debug("SCardTransmit : %s\n", pcsc_error_to_string(rc));
#endif
		FUNC_FAILS(-1, "SCardTransmit failed");
	}

	FUNC_RETURNS(lenr);
}



/**
 * checkForNewPCSCToken looks into a specific slot for a token.
 *
 * @param slot       Pointer to slot structure.
 *
 * @return
 *                   <P><TABLE>
 *                   <TR><TD>Code</TD><TD>Meaning</TD></TR>
 *                   <TR>
 *                   <TD>CKR_OK                                 </TD>
 *                   <TD>Success                                </TD>
 *                   </TR>
 *                   <TR>
 *                   <TD>CKR_HOST_MEMORY                        </TD>
 *                   <TD>Error getting memory (malloc)          </TD>
 *                   </TR>
 *                   <TR>
 *                   <TD>CKR_GENERAL_ERROR                      </TD>
 *                   <TD>Error opening slot directory           </TD>
 *                   </TR>
 *                   </TABLE></P>
 */
static int checkForNewPCSCToken(struct p11Slot_t *slot)
{
	struct p11Token_t *ptoken;
	int rc;
	DWORD dwActiveProtocol;

	FUNC_CALLED();

	if (slot->closed) {
		FUNC_RETURNS(CKR_TOKEN_NOT_PRESENT);
	}

	rc = SCardConnect(hContext, slot->readername, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &(slot->card), &dwActiveProtocol);

#ifdef DEBUG
	debug("SCardConnect (%i, %s): %s\n", slot->id, slot->readername, pcsc_error_to_string(rc));
#endif

	if (rc == SCARD_E_NO_SMARTCARD) {
		FUNC_RETURNS(CKR_TOKEN_NOT_PRESENT);
	}

	if (rc != SCARD_S_SUCCESS) {
		closeSlot(slot);
		FUNC_FAILS(CKR_DEVICE_ERROR, pcsc_error_to_string(rc));
	}

	rc = newToken(slot, &ptoken);

	if (rc != CKR_OK) {
		FUNC_FAILS(rc, "newToken failed()");
	}

	rc = addToken(slot, ptoken);

	FUNC_RETURNS(rc);
}



/**
 * checkForRemovedPCSCToken looks into a specific slot for a removed token.
 *
 * @param slot       Pointer to slot structure.
 *
 * @return
 *                   <P><TABLE>
 *                   <TR><TD>Code</TD><TD>Meaning</TD></TR>
 *                   <TR>
 *                   <TD>CKR_OK                                 </TD>
 *                   <TD>Success                                </TD>
 *                   </TR>
 *                   <TR>
 *                   <TD>CKR_HOST_MEMORY                        </TD>
 *                   <TD>Error getting memory (malloc)          </TD>
 *                   </TR>
 *                   <TR>
 *                   <TD>CKR_GENERAL_ERROR                      </TD>
 *                   <TD>Error opening slot directory           </TD>
 *                   </TR>
 *                   </TABLE></P>
 */
static int checkForRemovedPCSCToken(struct p11Slot_t *slot)
{
	int rc;

	FUNC_CALLED();

	if (slot->closed) {
		FUNC_RETURNS(CKR_TOKEN_NOT_PRESENT);
	}

	if (!slot->card) {
		FUNC_RETURNS(CKR_TOKEN_NOT_PRESENT);
	}

	rc = SCardStatus(slot->card, NULL, 0, 0, 0, 0, 0);

#ifdef DEBUG
	debug("SCardStatus : %s\n", pcsc_error_to_string(rc));
#endif

	if (rc == SCARD_S_SUCCESS) {
		FUNC_RETURNS(CKR_OK);
	} else if (rc == SCARD_W_REMOVED_CARD) {
		rc = removeToken(slot);
	} else {
		FUNC_FAILS(CKR_DEVICE_ERROR, "Error getting PC/SC card terminal status");
	}

	FUNC_RETURNS(rc);
}



int getPCSCToken(struct p11Slot_t *slot, struct p11Token_t **token)
{
	int rc;
	FUNC_CALLED();

	if (slot->token) {
		rc = checkForRemovedPCSCToken(slot);
	} else {
		rc = checkForNewPCSCToken(slot);
	}

	*token = slot->token;
	return rc;
}



int updatePCSCSlots(struct p11SlotPool_t *pool)
{
	struct p11Slot_t *slot;
	LPTSTR readers = NULL;
	DWORD cch = SCARD_AUTOALLOCATE;
	LPTSTR p;
	int rc, match;

	FUNC_CALLED();

	/*
	 * Create a context if not already done
	 */
	if (!hContext) {

		rc = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);

#ifdef DEBUG
		debug("SCardEstablishContext : %s\n", pcsc_error_to_string(rc));
#endif

		if (rc != SCARD_S_SUCCESS) {
			FUNC_FAILS(CKR_DEVICE_ERROR, "Could not establish context to PC/SC manager");
		}
	}

	rc = SCardListReaders(hContext, NULL, (LPTSTR)&readers, &cch);

#ifdef DEBUG
	debug("SCardListReaders : %s\n", pcsc_error_to_string(rc));
#endif

	if (rc != SCARD_S_SUCCESS) {
		FUNC_FAILS(CKR_DEVICE_ERROR, "Error listing PC/SC card terminals");
	}

	/* Determine the total number of readers */
	p = readers;
	while (*p != '\0') {
#ifdef DEBUG
		debug("%s\n", p);
#endif

		/* Check if the already have a slot for the reader */
		slot = pool->list;
		match = FALSE;
		while (slot) {
			if (strncmp(slot->readername, p, strlen(p)) == 0) {
				match = TRUE;
				break;
			}
			slot = slot->next;
		}

		/* Skip the reader as we already have a slot for it */
		if (match) {
			p += strlen(p) + 1;
			continue;
		}

		slot = (struct p11Slot_t *) calloc(1, sizeof(struct p11Slot_t));

		if (slot == NULL) {
			SCardFreeMemory(hContext, readers );
			FUNC_FAILS(CKR_HOST_MEMORY, "Out of memory");
		}

		strbpcpy(slot->info.slotDescription,
				(char *)p,
				sizeof(slot->info.slotDescription));

		strcpy(slot->readername, (char *)p);

		strbpcpy(slot->info.manufacturerID,
				"CardContact",
				sizeof(slot->info.manufacturerID));

		slot->info.hardwareVersion.minor = 0;
		slot->info.hardwareVersion.major = 0;

		slot->info.firmwareVersion.minor = 0;
		slot->info.firmwareVersion.major = 0;

		slot->info.flags = CKF_REMOVABLE_DEVICE | CKF_HW_SLOT;
		addSlot(context->slotPool, slot);

#ifdef DEBUG
		debug("Added slot (%lu, %s)\n", slot->id, slot->readername);
#endif
		p += strlen(p) + 1;
	}

    rc = SCardFreeMemory(hContext, readers );

#ifdef DEBUG
	debug("SCardFreeMemory : %s\n", pcsc_error_to_string(rc));
#endif

	if (rc != SCARD_S_SUCCESS) {
		FUNC_FAILS(CKR_DEVICE_ERROR, "Error freeing memory");
	}

	FUNC_RETURNS(CKR_OK);
}



int closePCSCSlot(struct p11Slot_t *slot)
{
	int rc;

	FUNC_CALLED();

#ifdef DEBUG
	debug("Trying to close slot (%i, %s)\n", slot->id, slot->readername);
#endif

	/* No token in slot */
	if (!slot->card) {
		slot->closed = TRUE;
		FUNC_RETURNS(CKR_OK);
	}

	rc =SCardDisconnect(slot->card, SCARD_UNPOWER_CARD);

#ifdef DEBUG
	debug("SCardDisconnect (%i, %s): %s\n", slot->id, slot->readername, pcsc_error_to_string(rc));
#endif

	slot->card = 0;
	slot->closed = TRUE;

	FUNC_RETURNS(CKR_OK);
}

#endif
