/*
 * CredaCash (TM) cryptocurrency and blockchain
 *
 * Copyright (C) 2015-2016 Creda Software, Inc.
 *
 * block.hpp
*/

#pragma once

#include <CCobjects.hpp>
#include <SmartBuf.hpp>

#define MAX_NWITNESSES				21

#define ROTATE_BLOCK_SIGNING_KEYS	0

//#define TEST_SIM_ALL_WITNESSES		1		// must rebuild all if changed

#ifndef TEST_SIM_ALL_WITNESSES
#define TEST_SIM_ALL_WITNESSES		0	// don't test
#endif

//#define TEST_SEQ_BLOCK_OID	1

#ifndef TEST_SEQ_BLOCK_OID
#define TEST_SEQ_BLOCK_OID		0	// don't test
#endif

typedef array<uint8_t, 256/8> block_signing_private_key_t;
typedef array<uint8_t, 256/8> block_signing_public_key_t;
typedef array<uint8_t, 512/8> block_signature_t;
typedef array<uint8_t, 512/8> block_hash_t;

#pragma pack(push, 1)

struct BlockWireHeader
{
	block_signature_t signature;
#if ROTATE_BLOCK_SIGNING_KEYS
	block_signing_public_key_t witness_next_signing_public_key;
#endif
	ccoid_t prior_oid;
	uint64_t level;
	uint64_t timestamp;
	uint8_t witness;
};

struct BlockSignedData
{
	block_hash_t prior_block_hash;
	block_hash_t block_hash;
#if ROTATE_BLOCK_SIGNING_KEYS
	block_signing_public_key_t witness_next_signing_public_key;
#endif
	uint32_t block_size;
	uint8_t witness;
};

#pragma pack(pop)

class BlockAux
{
public:
	ccoid_t oid;	// must be first for OidPtr() in CCobjects
	block_hash_t block_hash;
	uint32_t announce_time;
	uint16_t skip;
	bool marked_for_indelible;

	struct
	{
		uint16_t nwitnesses;
		uint16_t maxmal;
		uint16_t nconfsigs;
		uint16_t nseqconfsigs;
		uint16_t nskipconfsigs;

		uint16_t next_nwitnesses;	// note: after a block containing a "change-nwitnesses" command is committed, next_nwitnesses is changed in the block that caused or would cause the command to become committed
		uint16_t next_maxmal;

		block_signing_public_key_t signing_keys[MAX_NWITNESSES];

	} blockchain_params;

	struct
	{
		// used by witness:
		uint64_t score;
		uint16_t score_bits;
		uint16_t score_genstamp;

		#if TEST_SIM_ALL_WITNESSES
		array<block_signing_private_key_t, MAX_NWITNESSES> next_signing_private_key;
		#else
		array<block_signing_private_key_t, 1> next_signing_private_key;
		#endif

	} witness_params;

	void SetConfSigs();
	void SetHash(const block_hash_t& block_hash);
	void SetOid(const ccoid_t& oid);
};

class Block : public CCObject
{
	void CalcSkipScoreRecursive(const BlockWireHeader* last_indelible_wire, uint16_t genstamp, bool maltest, uint64_t& score, unsigned& scorebits);

public:
	BlockWireHeader* WireData()
	{
		return (BlockWireHeader*)BodyPtr();
	}

	const BlockWireHeader* WireData() const
	{
		return ((Block*)this)->WireData();
	}

	uint8_t* TxData()
	{
		return (uint8_t*)BodyPtr() + sizeof(BlockWireHeader);
	}

	const uint8_t* TxData() const
	{
		return ((Block*)this)->TxData();
	}

	unsigned TxDataSize() const
	{
		if (BodySize() > sizeof(BlockWireHeader))
			return BodySize() - sizeof(BlockWireHeader);
		else
			return 0;
	}

	bool HasTx() const
	{
		return TxDataSize() > 0;
	}

	SmartBuf GetPriorBlock() const
	{
		return SmartBuf(preamble.auxp[1]);
	}

	BlockAux* AuxPtr();

	const BlockAux* AuxPtr() const
	{
		return ((Block*)this)->AuxPtr();
	}

	void SetOrVerifyOid(bool bset);
	void CalcHash(block_hash_t& block_hash);
	void CalcOid(const block_hash_t& block_hash, ccoid_t& oid);

	BlockAux* SetupAuxBuf(SmartBuf smartobj);
	void SetPriorBlock(SmartBuf priorobj);
	void ChainToPriorBlock(SmartBuf priorobj);
	static unsigned ComputeSkip(unsigned prev_witness, unsigned next_witness, unsigned nwitnesses);

	bool CheckBadSigOrder(int top_witness) const;
	uint64_t CalcSkipScore(int top_witness, SmartBuf last_indelible_block, uint16_t genstamp, bool maltest);

	bool SignOrVerify(bool verify);
};
