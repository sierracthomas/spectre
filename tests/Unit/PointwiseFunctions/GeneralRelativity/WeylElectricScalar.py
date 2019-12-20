# Distributed under the MIT License.
# See LICENSE.txt for details.
import numpy as np

# does not pass all tests
#def weyl_electric_scalar(weyl_electric, inverse_spatial_metric):
#    return (np.einsum("ij,kl,ik,jl", weyl_electric, weyl_electric,
#                     inverse_spatial_metric, inverse_spatial_metric))
# does not pass all tests
#def weyl_electric_scalar(weyl_electric, inverse_spatial_metric):
#    return (np.einsum("ij, ij", weyl_electric, inverse_spatial_metric) *
#            np.einsum("kl, kl", weyl_electric, inverse_spatial_metric))
# creates wrong-sized resulting tensor
def weyl_electric_scalar(weyl_electric, inverse_spatial_metric):
    return (np.einsum("ij, kl", weyl_electric, weyl_electric) *
            np.einsum("ik, jl", inverse_spatial_metric, inverse_spatial_metric))
