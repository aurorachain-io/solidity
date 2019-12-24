/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file GasMeter.cpp
 * @author Christian <c@ethdev.com>
 * @date 2015
 */

#pragma once

#include <libevmasm/ExpressionClasses.h>
#include <libevmasm/AssemblyItem.h>

#include <libsolidity/interface/EVMVersion.h>

#include <ostream>
#include <tuple>

namespace dev
{
namespace eth
{

class KnownState;

namespace GasCosts
{
	static unsigned const stackLimit = 1024;
	static unsigned const tier0Gas = 0;
	static unsigned const tier1Gas = 1;
	static unsigned const tier2Gas = 1;
	static unsigned const tier3Gas = 2;
	static unsigned const tier4Gas = 3;
	static unsigned const tier5Gas = 4;
	static unsigned const tier6Gas = 7;
	static unsigned const tier7Gas = 0;
	inline unsigned extCodeGas(EVMVersion _evmVersion)
	{
		return _evmVersion >= EVMVersion::tangerineWhistle() ? 45 : 20;
	}
	inline unsigned balanceGas(EVMVersion _evmVersion)
	{
		return _evmVersion >= EVMVersion::tangerineWhistle() ? 25 : 20;
	}
	static unsigned const expGas = 2;
	inline unsigned expByteGas(EVMVersion _evmVersion)
	{
		return _evmVersion >= EVMVersion::spuriousDragon() ? 4 : 10;
	}
	static unsigned const keccak256Gas = 4;
	static unsigned const keccak256WordGas = 1;
	inline unsigned sloadGas(EVMVersion _evmVersion)
	{
		return _evmVersion >= EVMVersion::tangerineWhistle() ? 20 : 50;
	}
	static unsigned const sstoreSetGas = 1250;
	static unsigned const sstoreResetGas = 310;
	static unsigned const sstoreRefundGas = 950;
	static unsigned const jumpdestGas = 1;
	static unsigned const logGas = 24;
	static unsigned const logDataGas = 1;
	static unsigned const logTopicGas = 24;
	static unsigned const createGas = 2000;
	inline unsigned callGas(EVMVersion _evmVersion)
	{
		return _evmVersion >= EVMVersion::tangerineWhistle() ? 45 : 40;
	}
	static unsigned const callStipend = 1000;
	static unsigned const callValueTransferGas = 550;
	static unsigned const callNewAccountGas = 1600;
	inline unsigned selfdestructGas(EVMVersion _evmVersion)
	{
		return _evmVersion >= EVMVersion::tangerineWhistle() ? 350 : 0;
	}
	static unsigned const selfdestructRefundGas = 1500;
	static unsigned const memoryGas = 1;
	static unsigned const quadCoeffDiv = 1024;
	static unsigned const createDataGas = 12;
	static unsigned const txGas = 25000;
	static unsigned const txCreateGas = 20000;
	static unsigned const txDataZeroGas = 1;
	static unsigned const txDataNonZeroGas = 4;
	static unsigned const copyGas = 1;
	static unsigned const balanceOfGas = 50;
	static unsigned const transferAssetGas = 550;
}

/**
 * Class that helps computing the maximum gas consumption for instructions.
 * Has to be initialized with a certain known state that will be automatically updated for
 * each call to estimateMax. These calls have to supply strictly subsequent AssemblyItems.
 * A new gas meter has to be constructed (with a new state) for control flow changes.
 */
class GasMeter
{
public:
	struct GasConsumption
	{
		GasConsumption(unsigned _value = 0, bool _infinite = false): value(_value), isInfinite(_infinite) {}
		GasConsumption(u256 _value, bool _infinite = false): value(_value), isInfinite(_infinite) {}
		static GasConsumption infinite() { return GasConsumption(0, true); }

		GasConsumption& operator+=(GasConsumption const& _other);
		bool operator<(GasConsumption const& _other) const { return this->tuple() < _other.tuple(); }

		std::tuple<bool const&, u256 const&> tuple() const { return std::tie(isInfinite, value); }

		u256 value;
		bool isInfinite;
	};

	/// Constructs a new gas meter given the current state.
	GasMeter(std::shared_ptr<KnownState> const& _state, solidity::EVMVersion _evmVersion, u256 const& _largestMemoryAccess = 0):
		m_state(_state), m_evmVersion(_evmVersion), m_largestMemoryAccess(_largestMemoryAccess) {}

	/// @returns an upper bound on the gas consumed by the given instruction and updates
	/// the state.
	/// @param _inculdeExternalCosts if true, include costs caused by other contracts in calls.
	GasConsumption estimateMax(AssemblyItem const& _item, bool _includeExternalCosts = true);

	u256 const& largestMemoryAccess() const { return m_largestMemoryAccess; }

	/// @returns gas costs for simple instructions with constant gas costs (that do not
	/// change with EVM versions)
	static unsigned runGas(Instruction _instruction);

private:
	/// @returns _multiplier * (_value + 31) / 32, if _value is a known constant and infinite otherwise.
	GasConsumption wordGas(u256 const& _multiplier, ExpressionClasses::Id _value);
	/// @returns the gas needed to access the given memory position.
	/// @todo this assumes that memory was never accessed before and thus over-estimates gas usage.
	GasConsumption memoryGas(ExpressionClasses::Id _position);
	/// @returns the memory gas for accessing the memory at a specific offset for a number of bytes
	/// given as values on the stack at the given relative positions.
	GasConsumption memoryGas(int _stackPosOffset, int _stackPosSize);

	std::shared_ptr<KnownState> m_state;
	EVMVersion m_evmVersion;
	/// Largest point where memory was accessed since the creation of this object.
	u256 m_largestMemoryAccess;
};

inline std::ostream& operator<<(std::ostream& _str, GasMeter::GasConsumption const& _consumption)
{
	if (_consumption.isInfinite)
		return _str << "[???]";
	else
		return _str << std::dec << _consumption.value;
}


}
}
