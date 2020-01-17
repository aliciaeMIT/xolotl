#pragma once

#include <experimental/ReactionNetwork.h>
#include <experimental/PSIReaction.h>
#include <experimental/PSITraits.h>

namespace xolotlCore
{
namespace experimental
{
namespace detail
{
template <typename TSpeciesEnum>
class PSIReactionValidator;
}

template <typename TSpeciesEnum>
class PSIReactionNetwork :
    public ReactionNetwork<PSIReactionNetwork<TSpeciesEnum>>
{
    friend class ReactionNetwork<PSIReactionNetwork<TSpeciesEnum>>;
    friend class detail::ReactionNetworkWorker<PSIReactionNetwork<TSpeciesEnum>>;

public:
    using Superclass = ReactionNetwork<PSIReactionNetwork<TSpeciesEnum>>;
    using Subpaving = typename Superclass::Subpaving;
    using ReactionType = typename Superclass::ReactionType;
    using Composition = typename Superclass::Composition;
    using Species = typename Superclass::Species;
    using AmountType = typename Superclass::AmountType;

    static constexpr auto invalid = plsm::invalid<std::size_t>;

    using Superclass::Superclass;

private:
    double
    checkLatticeParameter(double latticeParameter)
    {
        if (latticeParameter <= 0.0) {
            return tungstenLatticeConstant;
        }
        return latticeParameter;
    }

    double
    checkImpurityRadius(double impurityRadius)
    {
        if (impurityRadius <= 0.0) {
            return heliumRadius;
        }
        return impurityRadius;
    }

    detail::PSIReactionValidator<Species>
    getReactionValidator() const noexcept
    {
        return {};
    }
};

namespace detail
{
template <typename TSpeciesEnum>
class PSIReactionValidator
{
public:
    using Network = PSIReactionNetwork<TSpeciesEnum>;
    using Subpaving = typename Network::Subpaving;
    using ClusterSet = typename Network::ReactionType::ClusterSet;

    KOKKOS_INLINE_FUNCTION
    void
    operator()(std::size_t i, std::size_t j, const Subpaving& subpaving,
        const UpperTriangle<Kokkos::pair<ClusterSet, ClusterSet> >& prodSet,
        const UpperTriangle<Kokkos::pair<ClusterSet, ClusterSet> >& dissSet) const;
};
}
}
}

#include <experimental/PSIClusterGenerator.h>
#include <experimental/PSIReactionNetwork.inl>
