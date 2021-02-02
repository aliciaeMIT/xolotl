#ifndef SORETDIFFUSIONHANDLER_H
#define SORETDIFFUSIONHANDLER_H

// Includes
#include <xolotl/core/modified/ISoretDiffusionHandler.h>
#include <xolotl/util/MathUtils.h>

namespace xolotl
{
namespace core
{
namespace modified
{
/**
 * This class realizes the IDiffusionHandler interface responsible for all
 * the physical parts for the diffusion of mobile clusters. It needs to have
 * subclasses implementing the compute diffusion methods.
 */
class SoretDiffusionHandler : public ISoretDiffusionHandler
{
protected:
	//! Collection of diffusing clusters.
	std::vector<std::size_t> diffusingClusters;

	/**
	 * The number of degrees of freedom in the network
	 */
	int dof;

	/**
	 * The x grid
	 */
	std::vector<double> xGrid;

	/**
	 * The heat flux in W.m-2
	 */
	double heatFlux;

	/**
	 * The heat conductivity
	 */
	double heatConductivity;

	/**
	 * The surface position
	 */
	int surfacePosition;

	/**
	 * The beta factor
	 */
	double beta;

	/**
	 * The He flux (nm-2 s-1)
	 */
	double J;

	/**
	 * Get the local heat factor
	 */
	double
	getLocalHeatFactor(int xi) const;

public:
	//! The Constructor
	SoretDiffusionHandler() :
		dof(0),
		surfacePosition(0),
		heatConductivity(0.0),
		heatFlux(0.0),
		beta(0.0),
		J(1.0e3)
	{
	}

	//! The Destructor
	~SoretDiffusionHandler()
	{
	}

	/**
	 * \see ISoretDiffusionHandler.h
	 */
	virtual void
	initialize(const network::IReactionNetwork& network,
		network::IReactionNetwork::SparseFillMap& ofill,
		network::IReactionNetwork::SparseFillMap& dfill,
		std::vector<double> grid) override
	{
		// Clear the index vector
		diffusingClusters.clear();

		// Set dof
		dof = network.getDOF();

		// Copy the grid
		xGrid = grid;

		// Consider each cluster
		for (std::size_t i = 0; i < network.getNumClusters(); i++) {
			auto cluster = network.getClusterCommon(i);

			// Get its diffusion factor and migration energy
			double diffFactor = cluster.getDiffusionFactor();

			// Don't do anything if the diffusion factor is 0.0
			if (util::equal(diffFactor, 0.0))
				continue;

			// Note that cluster is diffusing.
			diffusingClusters.emplace_back(i);

			// This cluster interacts with temperature now
			dfill[i].emplace_back(dof);
			ofill[i].emplace_back(dof);
		}

		return;
	}

	/**
	 * \see ISoretDiffusionHandler.h
	 */
	void
	setHeatFlux(double flux) override
	{
		heatFlux = flux;
	}

	/**
	 * \see ISoretDiffusionHandler.h
	 */
	void
	setHeatConductivity(double cond) override
	{
		heatConductivity = cond;
	}

	/**
	 * \see ISoretDiffusionHandler.h
	 */
	void
	updateSurfacePosition(int surfacePos) override
	{
		surfacePosition = surfacePos;
	}

	/**
	 * \see ISoretDiffusionHandler.h
	 */
	virtual void
	computeDiffusion(network::IReactionNetwork& network, double** concVector,
		double* updatedConcOffset, double hxLeft, double hxRight, int ix,
		double sy = 0.0, int iy = 0, double sz = 0.0, int iz = 0) const;

	/**
	 * \see ISoretDiffusionHandler.h
	 */
	virtual bool
	computePartialsForDiffusion(network::IReactionNetwork& network,
		double** concVector, double* val, int* indices, double hxLeft,
		double hxRight, int ix, double sy = 0.0, int iy = 0, double sz = 0.0,
		int iz = 0) const;
};
// end class SoretDiffusionHandler

} /* end namespace modified */
} /* end namespace core */
} /* end namespace xolotl */
#endif
