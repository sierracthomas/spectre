# Distributed under the MIT License.
# See LICENSE.txt for details.
import numpy as np

def weyl_electric_scalar(weyl_electric, inverse_spatial_metric):
    return (np.einsum("ij,kl,ik,jl", weyl_electric, weyl_electric,
                     inverse_spatial_metric, inverse_spatial_metric))
#def weyl_electric_scalar(weyl_electric, inverse_spatial_metric):
#    return (np.einsum("ij, ik", weyl_electric, inverse_spatial_metric) *
#            np.einsum("kl, jl", weyl_electric, inverse_spatial_metric))
