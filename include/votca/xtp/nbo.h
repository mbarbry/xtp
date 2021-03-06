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

#ifndef __VOTCA_XTP_NBO__H
#define	__VOTCA_XTP_NBO__H


#include <votca/tools/elements.h>
#include <votca/xtp/aobasis.h>
#include <votca/ctp/logger.h>
#include <votca/xtp/qmatom.h>


/**
* \brief Takes a list of atoms, and the corresponding density and overlap matrices and puts out a table of partial charges
*
* 
* 
*/



namespace votca { namespace xtp {
    
class NBO{
public:
    
    NBO(ctp::Logger *log){_log = log;}
   ~NBO(){};
       
   void EvaluateNBO(std::vector< QMAtom* >& _atomlist,const Eigen::MatrixXd  &_dmat,const AOBasis &_basis, BasisSet &bs);
  
private:
    
     ctp::Logger *_log;
     votca::tools::Elements _elements; 
    
    Eigen::MatrixXd IntercenterOrthogonalisation(Eigen::MatrixXd  &P,Eigen::MatrixXd  &Overlap,std::vector< QMAtom* >& _atomlist, BasisSet &bs);
  
};
}}

#endif /* __VOTCA_XTP_NBO_H */


