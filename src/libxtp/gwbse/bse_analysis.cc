/*
 *            Copyright 2009-2017 The VOTCA Development Team
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

#include <votca/xtp/bse.h>

using boost::format;
using namespace boost::filesystem;

namespace votca {
    namespace xtp {
       

        // +++++++++++++++++++++++++++++ //
        // MBPT MEMBER FUNCTIONS         //
        // +++++++++++++++++++++++++++++ //
        
        void BSE::Analyze_singlets() {
           
            // expectation values, contributions from e-h coupling
            std::vector<real_gwbse> _contrib_x(_bse_nprint, 0.0);
            std::vector<real_gwbse> _contrib_d(_bse_nprint, 0.0);
            std::vector<real_gwbse> _contrib_qp(_bse_nprint, 0.0);
            Analyze_eh_interaction_Singlet(_contrib_x, _contrib_d, _contrib_qp );
                      
            // for transition dipole moments
            CalcFreeTransition_Dipoles();
   
            CalcCoupledTransition_Dipoles();
            std::vector< Eigen::VectorXd > _popH;
            std::vector< Eigen::VectorXd > _popE;
            std::vector< Eigen::VectorXd > _Chrg;

            FragmentPopulations("singlet",_popH, _popE, _Chrg);
            _orbitals->setFragmentChargesSingEXC(_Chrg);
            _orbitals->setFragment_E_localisation_singlet(_popE);
            _orbitals->setFragment_H_localisation_singlet(_popH);
            // REPORTING
            const Eigen::VectorXd& _pop= _orbitals->getFragmentChargesGS();
            std::vector< tools::vec >& _transition_dipoles = _orbitals->TransitionDipoles();
            std::vector<double> oscs=_orbitals->Oscillatorstrengths();
            double hrt2ev=tools::conv::hrt2ev;
            CTP_LOG(ctp::logINFO, *_pLog) << (format("  ====== singlet energies (eV) ====== ")).str() << flush;
            for (int _i = 0; _i < _bse_nprint; _i++) {
                
                const tools::vec& trdip =_transition_dipoles[_i];
                double osc=oscs[_i];
                
                if (tools::globals::verbose) {
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("  S = %1$4d Omega = %2$+1.12f eV  lamdba = %3$+3.2f nm <FT> = %4$+1.4f <K_x> = %5$+1.4f <K_d> = %6$+1.4f")
                            % (_i + 1) % (hrt2ev * _bse_singlet_energies(_i)) % (1240.0 / (hrt2ev * _bse_singlet_energies(_i)))
                            % (hrt2ev * _contrib_qp[_i]) % (hrt2ev * _contrib_x[_i]) % (hrt2ev * _contrib_d[ _i ])).str() << flush;
                } else {
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("  S = %1$4d Omega = %2$+1.12f eV  lamdba = %3$+3.2f nm")
                            % (_i + 1) % (hrt2ev * _bse_singlet_energies(_i)) % (1240.0 / (hrt2ev * _bse_singlet_energies(_i)))).str() << flush;
                }
                
                CTP_LOG(ctp::logINFO, *_pLog) << (format("           TrDipole length gauge[e*bohr]  dx = %1$+1.4f dy = %2$+1.4f dz = %3$+1.4f |d|^2 = %4$+1.4f f = %5$+1.4f")
                        % trdip.getX() % trdip.getY() % trdip.getZ() % (trdip*trdip) % osc).str() << flush;
                for (unsigned _i_bse = 0; _i_bse < _bse_size; _i_bse++) {
                    // if contribution is larger than 0.2, print
                    double _weight = pow(_bse_singlet_coefficients(_i_bse, _i), 2) -  pow(_bse_singlet_coefficients_AR(_i_bse, _i), 2);
                    if (_weight > _min_print_weight) {
                        CTP_LOG(ctp::logINFO, *_pLog) << (format("           HOMO-%1$-3d -> LUMO+%2$-3d  : %3$3.1f%%")
                                % (_homo - _index2v[_i_bse]) % (_index2c[_i_bse] - _homo - 1) % (100.0 * _weight)).str() << flush;
                    }
                }
                 // results of fragment population analysis 
                if (_fragA > 0) {
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("           Fragment A -- hole: %1$5.1f%%  electron: %2$5.1f%%  dQ: %3$+5.2f  Qeff: %4$+5.2f")
                            % (100.0 * _popH[_i](0)) % (100.0 * _popE[_i](0)) % (_Chrg[_i](0)) % (_Chrg[_i](0) + _pop(0))).str() << flush;
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("           Fragment B -- hole: %1$5.1f%%  electron: %2$5.1f%%  dQ: %3$+5.2f  Qeff: %4$+5.2f")
                            % (100.0 * _popH[_i](1)) % (100.0 * _popE[_i](1)) % (_Chrg[_i](1)) % (_Chrg[_i](1) + _pop(1))).str() << flush;
                }
                
                CTP_LOG(ctp::logINFO, *_pLog)  << flush;
              
               
            }
            _bse_singlet_energies.resize(_bse_nmax);
            _transition_dipoles.resize(_bse_nprint);
            // storage to orbitals object
            if (_store_bse_singlets) {
                _bse_singlet_coefficients.resize(_bse_size, _bse_nmax);
                _bse_singlet_coefficients_AR.resize(_bse_size, _bse_nmax);
            } else {
                _bse_singlet_coefficients.resize(0, 0);
                _bse_singlet_coefficients_AR.resize(0, 0);
            }

            return;
        } 

        
        
        
         void BSE::Analyze_eh_interaction_Singlet(std::vector<real_gwbse>& _c_x, std::vector<real_gwbse>& _c_d, std::vector<real_gwbse>& _c_qp) {


            if (tools::globals::verbose) {
                for (int _i_exc = 0; _i_exc < _bse_nprint; _i_exc++) {

                    MatrixXfd _slice_R = _bse_singlet_coefficients.block(0,_i_exc,_bse_size,1);
                    MatrixXfd _res=_slice_R.transpose()*(_eh_d - _eh_qp)*_slice_R;
                    _c_d[_i_exc] = _res(0, 0);
                    _res = _slice_R.transpose()*_eh_x*_slice_R;
                    _c_x[_i_exc]  = 2.0 * _res(0, 0);
                    _c_qp[_i_exc] = _bse_singlet_energies(_i_exc) - _c_d[_i_exc]-_c_x[_i_exc] ;
                    
                    
                    if(_bse_singlet_coefficients_AR.cols()>0){
                      MatrixXfd _slice_AR = _bse_singlet_coefficients_AR.block(0,_i_exc, _bse_size,1);
      
                    // get anti-resonant contribution from direct Keh 
                    _res=_slice_AR.transpose()*(_eh_d - _eh_qp)*_slice_AR;
                    _c_d[_i_exc] -= _res(0, 0);
                    _c_qp[_i_exc] -= _res(0, 0);

                    // anti-resonant part
                    _res = _slice_AR.transpose()*_eh_x*_slice_AR;
                    _c_x[_i_exc]  -= 2.0 * _res(0, 0);
                    _c_qp[_i_exc] -= 2.0 * _res(0, 0);
                    }
                }
            }
            return;
         }
        

              
        void BSE::Analyze_eh_interaction_Triplet(std::vector<real_gwbse>& _c_d, std::vector<real_gwbse>& _c_qp) {

            if (tools::globals::verbose) {
                for (int _i_exc = 0; _i_exc < _bse_nprint; _i_exc++) {
                    MatrixXfd _slice_R = _bse_triplet_coefficients.block(0,_i_exc,_bse_size,1);
                    MatrixXfd _res=_slice_R.transpose()*(_eh_d - _eh_qp)*_slice_R;
                    _c_d[_i_exc] = _res(0, 0);
                    _c_qp[_i_exc] = _bse_triplet_energies(_i_exc) - _c_d[_i_exc] ;
                }
            }
            return;
        }
        
        void BSE::FragmentPopulations(const string& spin,std::vector< Eigen::VectorXd >& popH,
                std::vector< Eigen::VectorXd >& popE, std::vector< Eigen::VectorXd >& Crgs) {
                       
            // Mulliken fragment population analysis
            if (_fragA > 0) {

                // get overlap matrix for DFT basisset
                AOOverlap _dftoverlap;
                // Fill overlap
                _dftoverlap.Fill(_dftbasis);
                CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Filled DFT Overlap matrix of dimension: " << _dftoverlap.Matrix().rows() << flush;    
                // ground state populations
                Eigen::MatrixXd DMAT = _orbitals->DensityMatrixGroundState();
                
                Eigen::VectorXd nuccharges=_orbitals->FragmentNuclearCharges(_fragA);
                 Eigen::VectorXd pops=_orbitals->LoewdinPopulation(DMAT, _dftoverlap.Matrix(), _dftbasis.getAOBasisFragA());
                // population to electron charges and add nuclear charges         
                _orbitals->setFragmentChargesGS(nuccharges-pops); 
                for (int _i_state = 0; _i_state < _bse_nprint; _i_state++) {

                    // checking Density Matrices
                    std::vector< Eigen::MatrixXd > DMAT = _orbitals->DensityMatrixExcitedState(spin, _i_state);
                    // hole part
                     Eigen::VectorXd popsH=_orbitals->LoewdinPopulation(DMAT[0], _dftoverlap.Matrix(), _dftbasis.getAOBasisFragA());
                    popH.push_back(popsH);
                    // electron part
                     Eigen::VectorXd popsE=_orbitals->LoewdinPopulation(DMAT[1], _dftoverlap.Matrix(), _dftbasis.getAOBasisFragA());
                    popE.push_back(popsE);
                    // update effective charges
                     Eigen::VectorXd diff=popsH-popsE;
                    Crgs.push_back(diff);
                }
                CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Ran Excitation fragment population analysis " << flush;
            }
            return;
        }
        
        
        
        void BSE::CalcFreeTransition_Dipoles(){
            
             // Testing electric dipole AOMatrix
            AODipole _dft_dipole;
           
            
            _dft_dipole.Fill(_dftbasis);
            
            Eigen::MatrixXd _temp;
            // now transition dipole elements for free interlevel transitions
            _interlevel_dipoles.resize(3);
             Eigen::MatrixXd empty = _dft_orbitals.block(_bse_cmin,0, _bse_cmax-_bse_cmin + 1, _dftbasis.AOBasisSize());
             Eigen::MatrixXd occ = _dft_orbitals.block(_bse_vmin,0, _bse_vmax-_bse_vmin + 1, _dftbasis.AOBasisSize());
            for (int _i_comp = 0; _i_comp < 3; _i_comp++) {
               _interlevel_dipoles[ _i_comp ] = occ*_dft_dipole.Matrix()[_i_comp]*empty.transpose();
            }
            CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Calculated free interlevel transition dipole moments " << flush;   
           return; 
        }

       

        void BSE::CalcCoupledTransition_Dipoles() {

            double sqrt2 = sqrt(2.0);
            for (int _i_exc = 0; _i_exc < _bse_nprint; _i_exc++) {
                tools::vec _tdipole = vec(0,0,0);

                for (unsigned _v = 0; _v < _bse_vtotal; _v++) {
                    for (unsigned _c = 0; _c < _bse_ctotal; _c++) {
                        int index_vc = _bse_ctotal * _v + _c;
                        double factor=sqrt2 * _bse_singlet_coefficients(index_vc, _i_exc);
                        if(_do_full_BSE){
                            factor+=sqrt2 * _bse_singlet_coefficients_AR(index_vc, _i_exc);
                        }
                       
                        // The Transition dipole is sqrt2 bigger because of the spin, the excited state is a linear combination of 2 slater determinants, where either alpha or beta spin electron is excited
                        _tdipole.x()+= factor * _interlevel_dipoles[0](_v, _c);
                        _tdipole.y()+= factor * _interlevel_dipoles[1](_v, _c);
                        _tdipole.z()+= factor * _interlevel_dipoles[2](_v, _c);
                    }

                }
                
                _orbitals->TransitionDipoles().push_back(_tdipole);
            }
            return;
        }
      
    
        void BSE::Analyze_triplets() {

        
            std::vector<real_gwbse> _contrib_d(_bse_nprint, 0.0);
            std::vector<real_gwbse> _contrib_qp(_bse_nprint, 0.0);
            Analyze_eh_interaction_Triplet( _contrib_d, _contrib_qp);
            
            std::vector< Eigen::VectorXd > _popH;
            std::vector< Eigen::VectorXd > _popE;
            std::vector< Eigen::VectorXd > _Chrg;
            
            
            
            FragmentPopulations("triplet",_popH, _popE, _Chrg);
            _orbitals->setFragmentChargesTripEXC(_Chrg);
             _orbitals->setFragment_E_localisation_triplet(_popE);
            _orbitals->setFragment_H_localisation_triplet(_popH);
            const Eigen::VectorXd & _pop = _orbitals->getFragmentChargesGS();
            CTP_LOG(ctp::logINFO, *_pLog) << (format("  ====== triplet energies (eV) ====== ")).str() << flush;
            for (int _i = 0; _i < _bse_nprint; _i++) {

                if (tools::globals::verbose) {
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("  T = %1$4d Omega = %2$+1.12f eV  lamdba = %3$+3.2f nm <FT> = %4$+1.4f <K_d> = %5$+1.4f")
                            % (_i + 1) % (tools::conv::hrt2ev * _bse_triplet_energies(_i)) % (1240.0 / (tools::conv::hrt2ev * _bse_triplet_energies(_i)))
                            % (tools::conv::hrt2ev * _contrib_qp[_i]) % (tools::conv::hrt2ev * _contrib_d[ _i ])).str() << flush;
                } else {
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("  T = %1$4d Omega = %2$+1.12f eV  lamdba = %3$+3.2f nm")
                            % (_i + 1) % (tools::conv::hrt2ev * _bse_triplet_energies(_i)) % (1240.0 / (tools::conv::hrt2ev * _bse_triplet_energies(_i)))).str() << flush;
                }

                for (unsigned _i_bse = 0; _i_bse < _bse_size; _i_bse++) {
                    // if contribution is larger than 0.2, print
                    real_gwbse _weight = pow(_bse_triplet_coefficients(_i_bse, _i), 2);
                    if (_weight > _min_print_weight) {
                        CTP_LOG(ctp::logINFO, *_pLog) << (format("           HOMO-%1$-3d -> LUMO+%2$-3d  : %3$3.1f%%") % 
                                (_homo - _index2v[_i_bse]) % (_index2c[_i_bse] - _homo - 1) % (100.0 * _weight)).str() << flush;
                    }
                }
                // results of fragment population analysis 
                if (_fragA > 0) {
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("           Fragment A -- hole: %1$5.1f%%  electron: %2$5.1f%%  dQ: %3$+5.2f  Qeff: %4$+5.2f")
                            % (100.0 * _popH[_i](0)) % (100.0 * _popE[_i](0)) % (_Chrg[_i](0)) % (_Chrg[_i](0) + _pop(0))).str() << flush;
                    CTP_LOG(ctp::logINFO, *_pLog) << (format("           Fragment B -- hole: %1$5.1f%%  electron: %2$5.1f%%  dQ: %3$+5.2f  Qeff: %4$+5.2f")
                            % (100.0 * _popH[_i](1)) % (100.0 * _popE[_i](1)) % (_Chrg[_i](1)) % (_Chrg[_i](1) + _pop(1))).str() << flush;
                }
                CTP_LOG(ctp::logINFO, *_pLog) << (format("   ")).str() << flush;
            }

            // storage to orbitals object
            if (_store_bse_triplets) {
                _bse_triplet_energies.resize(_bse_nmax);
                _bse_triplet_coefficients.resize(_bse_size, _bse_nmax);
            } else {
                _bse_triplet_energies.resize(0);
                _bse_triplet_coefficients.resize(0, 0);
            }
            return;
        }


}};
