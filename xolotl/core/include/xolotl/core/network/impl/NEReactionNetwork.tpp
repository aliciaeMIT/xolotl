#pragma once

#include <xolotl/core/network/detail/impl/NucleationReactionGenerator.tpp>
#include <xolotl/core/network/detail/impl/ReSolutionReactionGenerator.tpp>
#include <xolotl/core/network/impl/NEClusterGenerator.tpp>
#include <xolotl/core/network/impl/NEReaction.tpp>
#include <xolotl/core/network/impl/ReactionNetwork.tpp>

namespace xolotl
{
namespace core
{
namespace network
{
void
NEReactionNetwork::checkTiles(const options::IOptions& options)
{
	auto maxXe = static_cast<AmountType>(options.getMaxImpurity() + 1);
	auto& tiles = this->getSubpaving().getTiles(plsm::onDevice);
	auto numClusters = tiles.extent(0);
	Kokkos::parallel_for(
		numClusters, KOKKOS_LAMBDA(const IndexType i) {
			auto clReg = tiles(i).getRegion();
			if (clReg[Species::Xe].end() > maxXe) {
				Region r({Ival{clReg[Species::Xe].begin(), maxXe}});
				auto id = tiles(i).getOwningZoneIndex();
				auto newTile = plsm::Tile<Region>(r, id);
				tiles(i) = newTile;
			}
		});
}

double
NEReactionNetwork::checkLatticeParameter(double latticeParameter)
{
	if (latticeParameter <= 0.0) {
		return uraniumDioxydeLatticeConstant;
	}
	return latticeParameter;
}

double
NEReactionNetwork::checkImpurityRadius(double impurityRadius)
{
	if (impurityRadius <= 0.0) {
		return xenonRadius;
	}
	return impurityRadius;
}

namespace detail
{
template <typename TTag>
KOKKOS_INLINE_FUNCTION
void
NEReactionGenerator::operator()(IndexType i, IndexType j, TTag tag) const
{
	using Species = typename NetworkType::Species;
	using Composition = typename NetworkType::Composition;
	using AmountType = typename NetworkType::AmountType;

	auto numClusters = this->getNumberOfClusters();

	// Get the composition of each cluster
	const auto& cl1Reg = this->getCluster(i).getRegion();
	const auto& cl2Reg = this->getCluster(j).getRegion();
	Composition lo1 = cl1Reg.getOrigin();
	Composition hi1 = cl1Reg.getUpperLimitPoint();
	Composition lo2 = cl2Reg.getOrigin();
	Composition hi2 = cl2Reg.getUpperLimitPoint();

	// General case
	Kokkos::pair<AmountType, AmountType> bounds;
	// Compute the bounds
	auto low = lo1[Species::Xe] + lo2[Species::Xe];
	auto high = hi1[Species::Xe] + hi2[Species::Xe] - 2;
	bounds = {low, high};

	// Look for potential product
	for (IndexType k = 0; k < numClusters; ++k) {
		// Get the composition
		const auto& prodReg = this->getCluster(k).getRegion();
		// Check the bounds
		if (prodReg[Species::Xe].begin() > bounds.second) {
			continue;
		}
		else if (prodReg[Species::Xe].end() - 1 < bounds.first) {
			continue;
		}

		if (cl1Reg.isSimplex() && cl2Reg.isSimplex() && lo1[Species::Xe] == 1 &&
			lo2[Species::Xe] == 1) {
			if (this->_clusterData.enableNucleation(0))
				this->addNucleationReaction(tag, {i, k});
			else
				this->addProductionReaction(tag, {i, j, k});
		}
		else
			this->addProductionReaction(tag, {i, j, k});

		if (!cl1Reg.isSimplex() && !cl2Reg.isSimplex()) {
			continue;
		}
		// Is the size of one of them one?
		if (lo1[Species::Xe] == 1 || lo2[Species::Xe] == 1) {
			this->addDissociationReaction(tag, {k, i, j});
			// Also add re-solution
			this->addReSolutionReaction(tag, {k, i, j});
		}
	}
}

inline ReactionCollection<NEReactionGenerator::NetworkType>
NEReactionGenerator::getReactionCollection() const
{
	ReactionCollection<NetworkType> ret(this->_clusterData.gridSize,
		this->getProductionReactions(), this->getDissociationReactions(),
		this->getReSolutionReactions(), this->getNucleationReactions());
	return ret;
}
} // namespace detail

inline detail::NEReactionGenerator
NEReactionNetwork::getReactionGenerator() const noexcept
{
	return detail::NEReactionGenerator{*this};
}

namespace detail
{
KOKKOS_INLINE_FUNCTION
void
NEClusterUpdater::updateDiffusionCoefficient(
	const ClusterData& data, IndexType clusterId, IndexType gridIndex) const
{
	// If the diffusivity is given
	if (data.migrationEnergy(clusterId) > 0.0) {
		// Intrinsic diffusion
		double kernel = -3.04 / (kBoltzmann * data.temperature(gridIndex));
		double D3 = 7.6e8 * exp(kernel); // nm2/s

		// We need the fission rate now
		double fissionRate = data.fissionRate(0) * 1.0e27; // #/m3/s

		// Athermal diffusion
		double D1 = (8e-40 * fissionRate) * 1.0e18; // nm2/s

		// Radiation-enhanced diffusion
		kernel = -1.2 / (kBoltzmann * data.temperature(gridIndex));
		double D2 =
			(5.6e-25 * sqrt(fissionRate) * exp(kernel)) * 1.0e18; // nm2/s

		data.diffusionCoefficient(clusterId, gridIndex) = D1 + D2 + D3;

		return;
	}

	data.diffusionCoefficient(clusterId, gridIndex) =
		data.diffusionFactor(clusterId);
}
} // namespace detail
} // namespace network
} // namespace core
} // namespace xolotl
