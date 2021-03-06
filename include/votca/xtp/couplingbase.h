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

#ifndef _VOTCA_XTP_COUPLINGBASE_H
#define	_VOTCA_XTP_COUPLINGBASE_H

#include <votca/xtp/orbitals.h>
#include <votca/ctp/logger.h>
#include <boost/format.hpp>
#include <votca/xtp/aomatrix.h>


namespace votca { namespace xtp {


/**
* \brief Base Class to derive DFT and BSE coupling from
*
* B. Baumeier, J. Kirkpatrick, D. Andrienko, 
* Phys. Chem. Chem. Phys., 12, 11103-11113, 2010
* 
*/

class CouplingBase
{
public:
    
    
    virtual void CalculateCouplings(const Orbitals& orbitalsA, 
                               const Orbitals& orbitalsB, 
                               Orbitals& orbitalsAB)=0;
    
    
    virtual void Initialize(tools::Property&)=0;
    
    virtual void Addoutput(tools::Property & type_summary,const Orbitals& orbitalsA, 
                               const Orbitals& orbitalsB)=0;
    
    void setLogger( ctp::Logger* pLog ) { _pLog = pLog; }
    
protected:
    ctp::Logger *_pLog;
    void CheckAtomCoordinates(const Orbitals& orbitalsA, const Orbitals& orbitalsB, const Orbitals& orbitalsAB);
   
   Eigen::MatrixXd CalculateOverlapMatrix( Orbitals& orbitalsAB); 
  

};

inline Eigen::MatrixXd CouplingBase::CalculateOverlapMatrix(Orbitals& orbitalsAB){
  BasisSet dftbasisset;
  AOBasis dftbasis;
  dftbasisset.LoadBasisSet(orbitalsAB.getDFTbasis());
  dftbasis.AOBasisFill(dftbasisset, orbitalsAB.QMAtoms());
  AOOverlap dftAOoverlap;
  dftAOoverlap.Fill(dftbasis);
  Eigen::MatrixXd overlapAB=dftAOoverlap.Matrix();
  return overlapAB;
}

inline void CouplingBase::CheckAtomCoordinates(const Orbitals& orbitalsA, 
                          const Orbitals& orbitalsB, const Orbitals& orbitalsAB){
  const std::vector<QMAtom*>& atomsA=orbitalsA.QMAtoms();
  const std::vector<QMAtom*>& atomsB=orbitalsB.QMAtoms();
  const std::vector<QMAtom*>& atomsAll = orbitalsAB.QMAtoms();
  bool coordinates_agree=true;
  for (unsigned i = 0; i < atomsAll.size(); i++) {
    QMAtom* dimer = atomsAll[i];
    QMAtom* monomer = NULL;
    
    if (i < atomsA.size()) {
      monomer = atomsA[i];
    } else if (i < atomsB.size() + atomsA.size()) {
      monomer = atomsB[i - atomsA.size()]; 
    } else {
      // Linker
      CTP_LOG(ctp::logERROR, *_pLog) << (boost::format("Neither Monomer A nor Monomer B contains "
              "atom %s on line %u. Hence, this atom is part of a linker.") %dimer->getType() %(i+1) ).str()<<std::flush;
      continue;
    }
    
    if(!monomer->getPos().isClose(dimer->getPos(), 0.001)){
        coordinates_agree=false;
    }
    
    if (monomer->getType() != dimer->getType()) {
      throw std::runtime_error("\nERROR: Atom types do not agree in dimer and monomers\n");
    }
  }
  
  if(!coordinates_agree){
        CTP_LOG(ctp::logINFO, *_pLog) << "======WARNING=======\n Coordinates of monomer "
              "and dimer atoms do not agree" << std::flush;
  }
}


}}

#endif	/* _VOTCA_XTP_DFTCOUPLING_H */


