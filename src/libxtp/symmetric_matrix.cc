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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "votca/xtp/symmetric_matrix.h"
#include <iostream>



namespace votca {
  namespace xtp {

    Symmetric_Matrix::Symmetric_Matrix(const Eigen::MatrixXd& full) : dimension(full.rows()) {
      assert(full.rows() == full.cols() && "Input matrix not quadratic");
      data.resize((dimension + 1) * dimension / 2);
      for (int i = 0; i < full.rows(); ++i) {
        for (int j = 0; j <= i; ++j) {
          this->operator()(i, j) = full(i, j);
        }
      }
    }

    std::ostream &operator<<(std::ostream &out, const Symmetric_Matrix& a) {

      out << "[" << a.dimension << "," << a.dimension << "]\n";
      for (unsigned i = 0; i < a.dimension; ++i) {
        for (unsigned j = 0; j <= i; ++j) {
          out << a(i, j);
          if (i == j) {
            out << "\n";
          } else {
            out << " ";
          }

        }
      }
      return out;
    }
    
     double Symmetric_Matrix::TraceofProd(const Symmetric_Matrix& a) const{
        assert(data.size()==a.data.size()&&"Matrices do not have same size");
        double result=0.0;
        
        for (size_t i=0;i<dimension;++i){
            result+=this->operator ()(i,i)*a.operator ()(i,i);
        }
        for (size_t i=0;i<dimension;++i){
            for (size_t j=0;j<i;++j){
                result+=2*this->operator ()(i,j)*a.operator ()(i,j);
        }
        }
        return result;
    }

    void Symmetric_Matrix::AddtoEigenMatrix(Eigen::MatrixXd& full, double factor) const {
      for (int j = 0; j < full.cols(); ++j) {
        const int start = (j * (j + 1)) / 2;
        for (int i = 0; i <= j; ++i) {
          full(i, j) += factor * data[start + i];
        }

        for (int i = j + 1; i < full.rows(); ++i) {
          const int index = (i * (i + 1)) / 2 + j;
          full(i, j) += factor * data[index];
        }
      }
      return;
    }

    Eigen::MatrixXd Symmetric_Matrix::FullMatrix()const{
      Eigen::MatrixXd result = Eigen::MatrixXd(dimension, dimension);
      for (int j = 0; j < result.cols(); ++j) {
        const int start = (j * (j + 1)) / 2;
        for (int i = 0; i <= j; ++i) {
          result(i, j) = data[start + i];
        }

        for (int i = j + 1; i < result.rows(); ++i) {
          const int index = (i * (i + 1)) / 2 + j;
          result(i, j) = data[index];
        }
      }
      return result;
    }
    
     size_t Symmetric_Matrix::Index(const size_t i,const size_t j)const{
        size_t index;
        if (i >= j) {
            index = (i * (i + 1)) / 2 + j;
        } else {
            index = (j * (j + 1)) / 2 + i;
        }
        return index;
    }



  }
}
