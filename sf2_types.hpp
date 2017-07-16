/*
 * This file is part of GBA Sound Riper
 * (c) 2012, 2014 Bregalad
 * This is free and open source software
 *
 * This class defines types closely related to SF2 specification
 */

#ifndef SF2_TYPES_HPP
#define SF2_TYPES_HPP
#include <stdint.h>

/* From the SF2 spec v2.1, page 7 */
// typedef uint8_t BYTE;
// typedef int8_t CHAR;
// typedef uint32_t DWORD;
// typedef int16_t SHORT;
// typedef uint16_t WORD;

/* From the SF2 spec v2.1 page 19 */
struct rangesType
{
	uint8_t byLo;
	uint8_t byHi;

	rangesType(uint8_t lo, uint8_t hi) : byLo(lo), byHi(hi)
	{}
};

// Two bytes that can handle either two 8-bit values or a single 16-bit value
union genAmountType
{
	rangesType ranges;
	int16_t shAmount;
	uint16_t wAmount;

	genAmountType(uint16_t value = 0) : wAmount(value)
	{}

	genAmountType(uint8_t lo, uint8_t hi) : ranges(lo, hi)
	{}
};

// SF2 v2.1 spec page 20
enum class SFSampleLink : uint16_t
{
	monoSample = 1,
	rightSample = 2,
	leftSample = 4,
	linkedSample = 8
};

// Generator's enumeration class
// SF2 v2.1 spec page 38
enum class SFGenerator : uint16_t
{
	_null = 0,
	startAddrsOffset = 0,
	endAddrsOffset = 1,
	startloopAddrsOffset = 2,
	endloopAddrsOffset = 3,
	startAddrsCoarseOffset = 4,
	modLfoToPitch = 5,
	vibLfoToPitch = 6,
	modEnvToPitch = 7,
	initialFilterFc = 8,
	initialFilterQ = 9,
	modLfoToFilterFc = 10,
	modEnvToFilterFc = 11,
	endAddrsCoarseOffset = 12,
	modLfoToVolume = 13,
	chorusEffectsSend = 15,
	reverbEffectsSend = 16,
	pan = 17,
	delayModLFO = 21,
	freqModLFO = 22,
	delayVibLFO = 23,
	freqVibLFO = 24,
	delayModEnv = 25,
	attackModEnv = 26,
	holdModEnv = 27,
	decayModEnv = 28,
	sustainModEnv = 29,
	releaseModEnv = 30,
	keynumToModEnvHold = 31,
	keynumToModEnvDecay = 32,
	delayVolEnv = 33,
	attackVolEnv = 34,
	holdVolEnv = 35,
	decayVolEnv = 36,
	sustainVolEnv = 37,
	releaseVolEnv = 38,
	keynumToVolEnvHold = 39,
	keynumToVolEnvDecay = 40,
	instrument = 41,
	keyRange = 43,
	velRange = 44,
	startloopAddrsCoarseOffset = 45,
	keynum = 46,
	velocity = 47,
	initialAttenuation = 48,
	endloopAddrsCoarseOffset = 50,
	coarseTune = 51,
	fineTune = 52,
	sampleID = 53,
	sampleModes = 54,
	scaleTuning = 56,
	exclusiveClass = 57,
	overridingRootKey = 58,
	endOper = 60
};

// Modulator's enumeration class
// SF2 v2.1 spec page 50
enum class SFModulator : uint16_t
{
	_null = 0,
	none = 0,
	noteOnVelocity = 1,
	noteOnKey = 2,
	polyPressure = 10,
	chnPressure = 13,
	pitchWheel = 14,
	ptchWeelSensivity = 16
};

// SF2 v2.1 spec page 52
enum class SFTransform : uint16_t
{
	_null = 0,
	linear = 0,
	concave = 1,
	convex = 2
};
#endif