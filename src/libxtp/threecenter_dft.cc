/* 
 *            Copyright 2009-2018 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */



#include <votca/xtp/threecenter.h>
#include <votca/xtp/symmetric_matrix.h>
#include <votca/xtp/eigen.h>
namespace votca {
  namespace xtp {

    void TCMatrix_dft::Fill(AOBasis& auxbasis, AOBasis& dftbasis, const Eigen::MatrixXd& V_sqrtm1) {

      for (int i = 0; i < auxbasis.AOBasisSize(); i++) {
        try {
          _matrix.push_back(Symmetric_Matrix(dftbasis.AOBasisSize()));
        } catch (std::bad_alloc& ba) {
          std::cerr << "Basisset/aux basis too large for 3c calculation. Not enough RAM. Caught bad alloc: " << ba.what() << std::endl;
          exit(0);
        }

      }
      #pragma omp parallel for schedule(dynamic)
      for (int is = dftbasis.getNumofShells()-1; is >=0; is--) {
        const Eigen::MatrixXd V=V_sqrtm1;
        const AOShell* dftshell = dftbasis.getShell(is);
        std::vector< Eigen::MatrixXd > block;
        for (int i = 0; i < dftshell->getNumFunc(); i++) {
          int size = dftshell->getStartIndex() + i+1;
          block.push_back(Eigen::MatrixXd::Zero(auxbasis.AOBasisSize(), size));
        }
        FillBlock(block, is, dftbasis, auxbasis);
        int offset = dftshell->getStartIndex();
        for (unsigned i = 0; i < block.size(); ++i) {
          Eigen::MatrixXd temp =V * block[i];
          for (int mu = 0; mu < temp.rows(); ++mu) {
            for (int j = 0; j < temp.cols(); ++j) {
              _matrix[mu](i + offset, j) = temp(mu, j);
            }
          }
        }
      }
      return;
    }

    /*
     * Determines the 3-center integrals for a given shell in the aux basis
     * by calculating the 3-center overlap integral of the functions in the
     * aux shell with ALL functions in the DFT basis set (FillThreeCenterOLBlock)
     */

    void TCMatrix_dft::FillBlock(std::vector< Eigen::MatrixXd >& block, int shellindex, const AOBasis& dftbasis, const AOBasis& auxbasis) {
      const AOShell* left_dftshell = dftbasis.getShell(shellindex);
      tensor3d::extent_gen extents;
      int start = left_dftshell->getStartIndex();
      // alpha-loop over the aux basis function
      for (const AOShell* shell_aux:auxbasis) {
        int aux_start = shell_aux->getStartIndex();


        for (int is = 0; is <= shellindex; is++) {

          const AOShell* shell_col = dftbasis.getShell(is);
          int col_start=shell_col->getStartIndex();
          tensor3d threec_block(extents[ range(0, shell_aux->getNumFunc()) ][ range(0, left_dftshell->getNumFunc()) ][ range(0, shell_col->getNumFunc())]);
          for (int i = 0; i < shell_aux->getNumFunc(); ++i) {
            for (int j = 0; j < left_dftshell->getNumFunc(); ++j) {
              for (int k = 0; k < shell_col->getNumFunc(); ++k) {
                threec_block[i][j][k] = 0.0;
              }
            }
          }

          bool nonzero = FillThreeCenterRepBlock(threec_block, shell_aux, left_dftshell, shell_col);
          if (nonzero) {

            for (int left = 0; left < left_dftshell->getNumFunc(); left++) {
              for (int aux = 0; aux < shell_aux->getNumFunc(); aux++) {
                for (int col = 0; col < shell_col->getNumFunc(); col++) {
                  //symmetry
                  if ((col_start + col)>(start + left)) {
                    break;
                  }
                  block[left](aux_start + aux, col_start + col) = threec_block[aux][left][col];
                }
              }
            }
          }
        }
      }
      return;
    }






  }
}
