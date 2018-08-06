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

// Overload of uBLAS prod function with MKL/GSL implementations

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <votca/ctp/logger.h>
#include <votca/tools/constants.h>
#include <votca/xtp/gwbse.h>
#include <votca/xtp/numerical_integrations.h>
#include <votca/xtp/qmpackagefactory.h>
#include <votca/xtp/ppm.h>
#include <votca/xtp/sigma.h>
#include <votca/xtp/bse.h>

using boost::format;
using namespace boost::filesystem;
using std::flush;
namespace votca {
namespace xtp {

// +++++++++++++++++++++++++++++ //
// GWBSE MEMBER FUNCTIONS        //
// +++++++++++++++++++++++++++++ //

void GWBSE::CleanUp() {}

void GWBSE::Initialize(tools::Property& options) {

#if (GWBSE_DOUBLE)
  CTP_LOG(ctp::logDEBUG, *_pLog) << " Compiled with full double support"
                                 << flush;
#else
  CTP_LOG(ctp::logDEBUG, *_pLog)
      << " Compiled with float/double mixture (standard)" << flush;
#endif

  std::string key = Identify();

  // getting level ranges
double qpminfactor=0;
  double qpmaxfactor=0;
  double rpamaxfactor=0;
  double bseminfactor=0;
  double bsemaxfactor=0;
std::string ranges = options.ifExistsReturnElseReturnDefault<std::string>(key + ".ranges",
                                                             "default");

  // now check validity, and get rpa, qp, and bse level ranges accordingly

  if (ranges == "factor") {
    // get factors
    rpamaxfactor = options.get(key + ".rpamax").as<double>();
    qpminfactor = options.get(key + ".qpmin").as<double>();
    qpmaxfactor = options.get(key + ".qpmax").as<double>();
    bseminfactor = options.get(key + ".bsemin").as<double>();
    bsemaxfactor = options.get(key + ".bsemax").as<double>();
  } else if (ranges == "explicit") {
    // get explicit numbers
    _rpamax = options.get(key + ".rpamax").as<unsigned>();
    _qpmin = options.get(key + ".qpmin").as<unsigned>();
    _qpmax = options.get(key + ".qpmax").as<unsigned>();
    _bse_vmin = options.get(key + ".bsemin").as<unsigned>();
    _bse_cmax = options.get(key + ".bsemax").as<unsigned>();
  } else if (ranges == "" || ranges == "default") {
    ranges = "default";
  } else if (ranges == "full") {
    ranges = "full";
  } else {
    std::cerr << "\nSpecified range option " << ranges << " invalid. ";
    throw std::runtime_error(
        "\nValid options are: default,factor,explicit,full");
  }
  
  
  /* Preparation of calculation parameters:
   *  - number of electrons -> index of HOMO
   *  - number of levels
   *  - highest level considered in RPA
   *  - lowest and highest level considered in GWA
   *  - lowest and highest level considered in BSE
   *  - number of excitations calculates in BSE
   */

  if (_orbitals.getNumberOfElectrons() <= 0) {
    throw std::runtime_error("no electrons in the system??");
  }

  _homo = _orbitals.getNumberOfElectrons() - 1;  // indexed from 0

 _reset_3c=options.ifExistsReturnElseReturnDefault<int>(
      key + ".rebuild_threecenter_freq", 5);

  _rpamin = 0;  // lowest index occ min(gwa%mmin, screening%nsum_low) ! always 1
  if (ranges == "default" || ranges == "full") {
    _rpamax = _orbitals.getNumberOfLevels() - 1;  // total number of levels
  } else if (ranges == "factor") {
    _rpamax = rpamaxfactor * _orbitals.getNumberOfLevels() -
              1;  // total number of levels
  }
  if (_rpamax > _orbitals.getNumberOfLevels() - 1) {
    _rpamax = _orbitals.getNumberOfLevels() - 1;
  }
  // convert _qpmin and _qpmax if needed
  if (ranges == "default") {
    _qpmin = 0;              // indexed from 0
    _qpmax = 2 * _homo + 1;  // indexed from 0
  } else if (ranges == "factor") {
    if (_orbitals.getNumberOfElectrons() -
            int(qpminfactor * _orbitals.getNumberOfElectrons()) - 1 <
        0) {
      _qpmin = 0;
    } else {
      _qpmin = _orbitals.getNumberOfElectrons() -
               int(qpminfactor * _orbitals.getNumberOfElectrons()) - 1;
    }
    _qpmax = _orbitals.getNumberOfElectrons() +
             int(qpmaxfactor * _orbitals.getNumberOfElectrons()) - 1;
  } else if (ranges == "explicit") {
    _qpmin -= 1;
    _qpmax -= 1;
  } else if (ranges == "full") {
    _qpmin = 0;
    _qpmax = _orbitals.getNumberOfLevels() - 1;
  }
  if (_qpmax > unsigned(_orbitals.getNumberOfLevels() - 1)) {
    _qpmax = _orbitals.getNumberOfLevels() - 1;
  }
  if (_qpmax> _rpamax){
    _qpmax=_rpamax;
  }
  
 

  // set BSE band range indices
  // anything else would be stupid!
  _bse_vmax = _homo;
  _bse_cmin = _homo + 1;

  if (ranges == "default") {
    _bse_vmin = 0;              // indexed from 0
    _bse_cmax = 2 * _homo + 1;  // indexed from 0
  } else if (ranges == "factor") {
    _bse_vmin = _orbitals.getNumberOfElectrons() -
                int(bseminfactor * _orbitals.getNumberOfElectrons()) - 1;
    if (_orbitals.getNumberOfElectrons() -
            int(bseminfactor * _orbitals.getNumberOfElectrons()) - 1 <
        0) {
      _bse_vmin = 0;
    }
    _bse_cmax = _orbitals.getNumberOfElectrons() +
                int(bsemaxfactor * _orbitals.getNumberOfElectrons()) - 1;
  } else if (ranges == "explicit") {
    _bse_vmin -= 1;
    _bse_cmax -= 1;
  } else if (ranges == "full") {
    _bse_vmin = 0;
    _bse_cmax = _orbitals.getNumberOfLevels() - 1;
  }
  if (_bse_cmax > unsigned(_orbitals.getNumberOfLevels() - 1)) {
    _bse_cmax = _orbitals.getNumberOfLevels() - 1;
  }

  bool ignore_corelevels = options.ifExistsReturnElseReturnDefault<bool>(
      key + ".ignore_corelevels", false);
  
  
   unsigned _ignored_corelevels = 0;
  if (ignore_corelevels) {
    if(!_orbitals.hasECP()){
      BasisSet basis;
      basis.LoadPseudopotentialSet("corelevels");//
      unsigned coreElectrons=0;
      for(const auto& atom:_orbitals.QMAtoms()){
        coreElectrons+=basis.getElement(atom->getType()).getNcore();   
      }
       _ignored_corelevels = coreElectrons/2;
    }
   
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Ignoring "
                                   << _ignored_corelevels << " core levels "
                                   << flush;
  }
  // autoignore core levels in QP
  if (ignore_corelevels && (_qpmin < _ignored_corelevels)) {
    _qpmin = _ignored_corelevels;
  }
  // autoignore core levels in BSE
  if (ignore_corelevels && (_bse_vmin < _ignored_corelevels)) {
    _bse_vmin = _ignored_corelevels;
  }

  int bse_vtotal = _bse_vmax - _bse_vmin + 1;
  int bse_ctotal = _bse_cmax - _bse_cmin + 1;
  int bse_size = bse_vtotal * bse_ctotal;


  // some QP - BSE consistency checks are required
  if (_bse_vmin < _qpmin) _qpmin = _bse_vmin;
  if (_bse_cmax > _qpmax) _qpmax = _bse_cmax;

  _qptotal = _qpmax - _qpmin + 1;
 

  // information for hybrid DFT

  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Set RPA level range ["
                                 << _rpamin + 1 << ":" << _rpamax + 1 << "]"
                                 << flush;
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Set QP  level range ["
                                 << _qpmin + 1 << ":" << _qpmax + 1 << "]"
                                 << flush;
  CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp() << " Set BSE level range occ[" << _bse_vmin + 1 << ":"
      << _bse_vmax + 1 << "]  virt[" << _bse_cmin + 1 << ":" << _bse_cmax + 1
      << "]" << flush;
  CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp() << " BSE Hamiltonian has size " << bse_size << "x"
      << bse_size << flush;
  
  _bse_maxeigenvectors =
      options.ifExistsReturnElseReturnDefault<int>(key + ".exctotal", 25);
   if (_bse_maxeigenvectors > int(bse_size) || _bse_maxeigenvectors < 0) _bse_maxeigenvectors = bse_size;
  _fragA = options.ifExistsReturnElseReturnDefault<int>(key + ".fragment", -1);

  std::string BSEtype =
      options.ifExistsReturnElseReturnDefault<std::string>(key + ".BSEtype", "TDA");

  if (BSEtype == "full") {
    _do_full_BSE = true;
    CTP_LOG(ctp::logDEBUG, *_pLog) << " BSE type: full" << flush;
  } else {
    _do_full_BSE = false;
    CTP_LOG(ctp::logDEBUG, *_pLog) << " BSE type: TDA" << flush;
  }

  _openmp_threads =
      options.ifExistsReturnElseReturnDefault<int>(key + ".openmp", 0);

  if (options.exists(key + ".vxc")) {
    _doVxc =
        options.ifExistsReturnElseThrowRuntimeError<bool>(key + ".vxc.dovxc");
    if (_doVxc) {
      _functional = options.ifExistsReturnElseThrowRuntimeError<std::string>(
          key + ".vxc.functional");
      _grid = options.ifExistsReturnElseReturnDefault<std::string>(
          key + ".vxc.grid", "medium");
    }
  }

  _auxbasis_name =
      options.ifExistsReturnElseThrowRuntimeError<std::string>(key + ".gwbasis");
  _dftbasis_name =
      options.ifExistsReturnElseThrowRuntimeError<std::string>(key + ".dftbasis");

  _shift = options.ifExistsReturnElseThrowRuntimeError<double>(key + ".shift");
  _g_sc_limit = options.ifExistsReturnElseReturnDefault<double>(
      key + ".g_sc_limit",
      0.00001);  // convergence criteria for qp iteration [Hartree]]
  _g_sc_max_iterations = options.ifExistsReturnElseReturnDefault<int>(
      key + ".g_sc_max_iterations",
      40);  // convergence criteria for qp iteration [Hartree]]

  _gw_sc_max_iterations = options.ifExistsReturnElseReturnDefault<int>(
      key + ".gw_sc_max_iterations",
      20);  // convergence criteria for qp iteration [Hartree]]

  _gw_sc_limit = options.ifExistsReturnElseReturnDefault<double>(
      key + ".gw_sc_limit", 0.00001);  // convergence criteria for shift it
  _iterate_gw = false;
  std::string _shift_type =
      options.ifExistsReturnElseThrowRuntimeError<std::string>(key + ".shift_type");
  if (_shift_type != "fixed") {
    _iterate_gw = true;
  }
  CTP_LOG(ctp::logDEBUG, *_pLog) << " Shift: " << _shift_type << flush;
  CTP_LOG(ctp::logDEBUG, *_pLog) << " g_sc_limit [Hartree]: " << _g_sc_limit
                                 << flush;
  if (_iterate_gw) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " gw_sc_limit [Hartree]: " << _gw_sc_limit
                                   << flush;
  }
  _min_print_weight = options.ifExistsReturnElseReturnDefault<double>(
      key + ".bse_print_weight",
      0.2);  // print exciton WF composition weight larger that thin minimum

  // setting some defaults
  _do_qp_diag = false;
  _do_bse_singlets = false;
  _do_bse_triplets = false;
  // possible tasks
  // diagQP, singlets, triplets, all, ibse
  std::string _tasks_string =
      options.ifExistsReturnElseThrowRuntimeError<std::string>(key + ".tasks");
  if (_tasks_string.find("all") != std::string::npos) {
    _do_qp_diag = true;
    _do_bse_singlets = true;
    _do_bse_triplets = true;
  }
  if (_tasks_string.find("qpdiag") != std::string::npos) _do_qp_diag = true;
  if (_tasks_string.find("singlets") != std::string::npos)
    _do_bse_singlets = true;
  if (_tasks_string.find("triplets") != std::string::npos)
    _do_bse_triplets = true;
  _store_eh_interaction = false;
  _do_bse_diag = true;
  // special construction for ibse mode
  if (_tasks_string.find("igwbse") != std::string::npos) {
    _do_qp_diag = false;   // no qp diagonalization
    _do_bse_diag = false;  // no diagonalization of BSE Hamiltonian
    _store_eh_interaction = true;
  }

  // possible storage
  // qpPert, qpdiag_energies, qp_diag_coefficients, bse_singlet_energies,
  // bse_triplet_energies, bse_singlet_coefficients, bse_triplet_coefficients
  _store_qp_pert = true;

  _store_qp_diag = false;
  _store_bse_triplets = false;
  _store_bse_singlets = false;
  std::string _store_string =
      options.ifExistsReturnElseThrowRuntimeError<std::string>(key + ".store");
  if ((_store_string.find("all") != std::string::npos) ||
      (_store_string.find("") != std::string::npos)) {
    // store according to tasks choice
    if (_do_qp_diag) _store_qp_diag = true;
    if (_do_bse_singlets && _do_bse_diag) _store_bse_singlets = true;
    if (_do_bse_triplets && _do_bse_diag) _store_bse_triplets = true;
  }
  if (_store_string.find("qpdiag") != std::string::npos) _store_qp_diag = true;
  if (_store_string.find("singlets") != std::string::npos)
    _store_bse_singlets = true;
  if (_store_string.find("triplets") != std::string::npos)
    _store_bse_triplets = true;
  if (_store_string.find("ehint") != std::string::npos)
    _store_eh_interaction = true;

  CTP_LOG(ctp::logDEBUG, *_pLog) << " Tasks: " << flush;
  if (_do_qp_diag) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " qpdiag " << flush;
  }
  if (_do_bse_singlets) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " singlets " << flush;
  }
  if (_do_bse_triplets) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " triplets " << flush;
  }
  CTP_LOG(ctp::logDEBUG, *_pLog) << " Store: " << flush;
  if (_store_qp_diag) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " qpdiag " << flush;
  }
  if (_store_bse_singlets) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " singlets " << flush;
  }
  if (_store_bse_triplets) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " triplets " << flush;
  }
  if (_store_eh_interaction) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << " ehint " << flush;
  }

  return;
}

void GWBSE::addoutput(tools::Property& _summary) {

  const double hrt2ev = tools::conv::hrt2ev;
  tools::Property _gwbse_summary = _summary.add("GWBSE", "");
  _gwbse_summary.setAttribute("units", "eV");
  _gwbse_summary.setAttribute("DeltaHLGap",
                               (format("%1$+1.6f ") % (_shift * hrt2ev)).str());

  _gwbse_summary.setAttribute(
      "DFTEnergy", (format("%1$+1.6f ") % _orbitals.getQMEnergy()).str());
  int printlimit = _bse_maxeigenvectors;  // I use this to determine how much is printed,
                                 // I do not want another option to pipe through

  tools::Property &_dft_summary = _gwbse_summary.add("dft", "");
  _dft_summary.setAttribute("HOMO", _homo);
  _dft_summary.setAttribute("LUMO", _homo + 1);
  
  for (unsigned state = _qpmin; state < _qpmax+1; state++) {

     tools::Property& _level_summary = _dft_summary.add("level", "");
    _level_summary.setAttribute("number", state);
    _level_summary.add("dft_energy",
                        (format("%1$+1.6f ") %
                         ((_orbitals.MOEnergies())(_qpmin + state) * hrt2ev))
                            .str());
    _level_summary.add(
        "gw_energy",
        (format("%1$+1.6f ") % (_orbitals.QPpertEnergies()(_qpmin + state) * hrt2ev)).str());

    if (_do_qp_diag) {
      _level_summary.add(
          "qp_energy",
          (format("%1$+1.6f ") % (_orbitals.QPdiagEnergies()(_qpmin + state) * hrt2ev))
              .str());
    }
  }

  if (_do_bse_singlets) {
     tools::Property &_singlet_summary = _gwbse_summary.add("singlets", "");
    for (int state = 0; state < _bse_maxeigenvectors; ++state) {
       tools::Property &_level_summary = _singlet_summary.add("level", "");
      _level_summary.setAttribute("number", state + 1);
      _level_summary.add("omega", (format("%1$+1.6f ") %
                                    (_orbitals.BSESingletEnergies()(state) * hrt2ev))
                                       .str());
      if (_orbitals.hasTransitionDipoles()) {

        const tools::vec &dipoles = (_orbitals.TransitionDipoles())[state];
        double f = 2 * dipoles * dipoles * _orbitals.BSESingletEnergies()(state) / 3.0;

        _level_summary.add("f", (format("%1$+1.6f ") % f).str());
         tools::Property& _dipol_summary = _level_summary.add(
            "Trdipole", (format("%1$+1.4f %2$+1.4f %3$+1.4f") % dipoles.getX() %
                         dipoles.getY() % dipoles.getZ())
                            .str());
        _dipol_summary.setAttribute("unit", "e*bohr");
        _dipol_summary.setAttribute("gauge", "length");
      }
    }
  }
  if (_do_bse_triplets) {
     tools::Property &_triplet_summary = _gwbse_summary.add("triplets", "");
    for (int state = 0; state < printlimit; ++state) {

       tools::Property &_level_summary = _triplet_summary.add("level", "");
      _level_summary.setAttribute("number", state + 1);
      _level_summary.add("omega", (format("%1$+1.6f ") %
                                    (_orbitals.BSETripletEnergies()(state) * hrt2ev))
                                       .str());
    }
  }
  return;
}

/*
 *    Many-body Green's fuctions theory implementation
 *
 *  data required from orbitals file
 *  - atomic coordinates
 *  - DFT molecular orbitals (energies and coeffcients)
 *  - DFT exchange-correlation potential matrix in atomic orbitals
 *  - number of electrons, number of levels
 *

 */

Eigen::MatrixXd GWBSE::CalculateVXC(const AOBasis& dftbasis){
  
  Eigen::MatrixXd vxc_ao;
  if (_orbitals.hasAOVxc()) {
    if (_doVxc) {
      if(_orbitals.getQMpackage()=="xtp"){
        CTP_LOG(ctp::logDEBUG, *_pLog)
              << ctp::TimeStamp()
              << " Taking VXC from xtp DFT run."
              << flush;
      }else{
      CTP_LOG(ctp::logDEBUG, *_pLog)
              << ctp::TimeStamp()
              << " There is already a Vxc matrix loaded from DFT, did you maybe "
              "run a DFT code with outputVxc?\n I will take the external "
              "implementation"
              << flush;
      }
      _doVxc = false;
    }
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
            << " Loaded external Vxc matrix" << flush;
    vxc_ao = _orbitals.AOVxc();
  } else if (_doVxc) {
    
    NumericalIntegration _numint;
    _numint.setXCfunctional(_functional);
    double ScaHFX_temp = _numint.getExactExchange(_functional);
    if (ScaHFX_temp != _orbitals.getScaHFX()) {
      throw std::runtime_error(
              (boost::format("GWBSE exact exchange a=%s differs from qmpackage "
              "exact exchange a=%s, probably your functionals are "
              "inconsistent") %
              ScaHFX_temp %  _orbitals.getScaHFX())
              .str());
    }
    _numint.GridSetup(_grid, _orbitals.QMAtoms(),&dftbasis);
    CTP_LOG(ctp::logDEBUG, *_pLog)
            << ctp::TimeStamp()
            << " Setup grid for integration with gridsize: " << _grid << " with "
            << _numint.getGridSize() << " points, divided into "
            << _numint.getBoxesSize() << " boxes" << flush;
    CTP_LOG(ctp::logDEBUG, *_pLog)
            << ctp::TimeStamp() << " Integrating Vxc in VOTCA with functional "
            << _functional << flush;
    Eigen::MatrixXd DMAT = _orbitals.DensityMatrixGroundState();
    
    vxc_ao = _numint.IntegrateVXC(DMAT);
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
            << " Calculated Vxc in VOTCA" << flush;
    
  } else {
    throw std::runtime_error(
            "So your DFT data contains no Vxc, if you want to proceed use the "
            "dovxc option.");
  }
  
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
          << " Set hybrid exchange factor: " <<  _orbitals.getScaHFX()
          << flush;
  
  // now get expectation values but only for those in _qpmin:_qpmax range
  Eigen::MatrixXd _mos = _orbitals.MOCoefficients().block(0,_qpmin,_orbitals.MOCoefficients().rows(),_qptotal);
  
  Eigen::MatrixXd vxc =_mos.transpose()*vxc_ao*_mos;
  CTP_LOG(ctp::logDEBUG, *_pLog)
          << ctp::TimeStamp()
          << " Calculated exchange-correlation expectation values " << flush;
  return vxc;
}

void GWBSE::PrintGWA_Energies(const Eigen::MatrixXd& vxc, const Sigma& sigma,const Eigen::VectorXd& _dft_energies){
  const Eigen::VectorXd& gwa_energies=sigma.getGWAEnergies();

  CTP_LOG(ctp::logINFO, *_pLog)
          << (format(
          "  ====== Perturbative quasiparticle energies (Hartree) ====== "))
          .str()
          << flush;
  CTP_LOG(ctp::logINFO, *_pLog)
          << (format("   DeltaHLGap = %1$+1.6f Hartree") % _shift).str() << flush;
  
  for (unsigned i = 0; i < _qptotal; i++) {
    if ((i + _qpmin) == _homo) {
      CTP_LOG(ctp::logINFO, *_pLog)
              << (format("  HOMO  = %1$4d DFT = %2$+1.4f VXC = %3$+1.4f S-X = "
              "%4$+1.4f S-C = %5$+1.4f GWA = %6$+1.4f") %
              (i + _qpmin + 1) % _dft_energies(i + _qpmin) % vxc(i, i) %
              sigma.x(i) % sigma.c(i) % gwa_energies(i + _qpmin))
              .str()
              << flush;
    } else if ((i + _qpmin) == _homo + 1) {
      CTP_LOG(ctp::logINFO, *_pLog)
              << (format("  LUMO  = %1$4d DFT = %2$+1.4f VXC = %3$+1.4f S-X = "
              "%4$+1.4f S-C = %5$+1.4f GWA = %6$+1.4f") %
              (i + _qpmin + 1) % _dft_energies(i + _qpmin) % vxc(i, i) %
              sigma.x(i) % sigma.c(i) % gwa_energies(i + _qpmin))
              .str()
              << flush;
      
    } else {
      CTP_LOG(ctp::logINFO, *_pLog)
              << (format("  Level = %1$4d DFT = %2$+1.4f VXC = %3$+1.4f S-X = "
              "%4$+1.4f S-C = %5$+1.4f GWA = %6$+1.4f") %
              (i + _qpmin + 1) % _dft_energies(i + _qpmin) % vxc(i, i) %
              sigma.x(i) % sigma.c(i) % gwa_energies(i + _qpmin))
              .str()
              << flush;
    }
  }
  return;
}

void GWBSE::PrintQP_Energies(const Eigen::VectorXd& gwa_energies, const Eigen::VectorXd& qp_diag_energies){
  CTP_LOG(ctp::logDEBUG, *_pLog)
          << ctp::TimeStamp() << " Full quasiparticle Hamiltonian  " << flush;
  CTP_LOG(ctp::logINFO, *_pLog)
          << (format("  ====== Diagonalized quasiparticle energies (Hartree) "
          "====== "))
          .str()
          << flush;
  for (unsigned _i = 0; _i < _qptotal; _i++) {
    if ((_i + _qpmin) == _homo) {
      CTP_LOG(ctp::logINFO, *_pLog)
              << (format("  HOMO  = %1$4d PQP = %2$+1.4f DQP = %3$+1.4f ") %
              (_i + _qpmin + 1) % gwa_energies(_i + _qpmin) %
              qp_diag_energies(_i))
              .str()
              << flush;
    } else if ((_i + _qpmin) == _homo + 1) {
      CTP_LOG(ctp::logINFO, *_pLog)
              << (format("  LUMO  = %1$4d PQP = %2$+1.4f DQP = %3$+1.4f ") %
              (_i + _qpmin + 1) % gwa_energies(_i + _qpmin) %
              qp_diag_energies(_i))
              .str()
              << flush;
      
    } else {
      CTP_LOG(ctp::logINFO, *_pLog)
              << (format("  Level = %1$4d PQP = %2$+1.4f DQP = %3$+1.4f ") %
              (_i + _qpmin + 1) % gwa_energies(_i + _qpmin) %
              qp_diag_energies(_i))
              .str()
              << flush;
    }
  }
  return;
}

bool GWBSE::Evaluate() {

// set the parallelization
#ifdef _OPENMP
  if (_openmp_threads > 0) {
    omp_set_num_threads(_openmp_threads);
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Using "
                                   << omp_get_max_threads() << " threads"
                                   << flush;
  }
#endif
  
  if(tools::globals::VOTCA_MKL){
     CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " Using MKL overload for Eigen "<< flush;
  }else{
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " Using native Eigen implementation, no BLAS overload "<< flush;
  }
  
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Molecule Coordinates [A] " << flush;
  for (QMAtom* atom:_orbitals.QMAtoms()) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << "\t\t " << atom->getType() << " " << atom->getPos() * tools::conv::bohr2ang << " " << flush;
  }
  
  
  /* check which QC program was used for the DFT run
   * -> implicit info about MO coefficient storage order
   */
  std::string _dft_package = _orbitals.getQMpackage();
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " DFT data was created by " << _dft_package
                                 << flush;
  
   // store information in _orbitals for later use
  _orbitals.setRPAindices(_rpamin, _rpamax);
  _orbitals.setGWAindices(_qpmin, _qpmax);
  _orbitals.setBSEindices(_bse_vmin, _bse_vmax, _bse_cmin, _bse_cmax,
                           _bse_maxeigenvectors);


            
  // load DFT basis set (element-wise information) from xml file
  BasisSet dftbs;

  if (_dftbasis_name != _orbitals.getDFTbasis()) {
    throw std::runtime_error(
        "Name of the Basisset from .orb file: " + _orbitals.getDFTbasis() +
        " and from GWBSE optionfile " + _dftbasis_name + " do not agree.");
  }

  dftbs.LoadBasisSet(_dftbasis_name);
  _orbitals.setDFTbasis(_dftbasis_name);
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Loaded DFT Basis Set "
                                 << _dftbasis_name << flush;

  // fill DFT AO basis by going through all atoms
  AOBasis dftbasis;
  dftbasis.AOBasisFill(dftbs, _orbitals.QMAtoms(), _fragA);
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " Filled DFT Basis of size "
                                 << dftbasis.AOBasisSize() << flush;
  if (dftbasis.getAOBasisFragB() > 0  && dftbasis.getAOBasisFragA()>0) {
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " FragmentA size "
                                   << dftbasis.getAOBasisFragA() << flush;
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " FragmentB size "
                                   << dftbasis.getAOBasisFragB()<< flush;
  }

  if (_do_full_BSE)
    _orbitals.setBSEtype("full");
  else {
    _orbitals.setBSEtype("TDA");
  }
  

  // process the DFT data
  // a) form the expectation value of the XC functional in MOs


  Eigen::MatrixXd vxc=CalculateVXC(dftbasis);

  /// ------- actual calculation begins here -------

  // load auxiliary basis set (element-wise information) from xml file
  BasisSet auxbs;
  auxbs.LoadBasisSet(_auxbasis_name);
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Loaded Auxbasis Set "
                                 << _auxbasis_name << flush;

  // fill auxiliary AO basis by going through all atoms
  AOBasis auxbasis;
  auxbasis.AOBasisFill(auxbs, _orbitals.QMAtoms());
  _orbitals.setAuxbasis(_auxbasis_name);
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " Filled Auxbasis of size "
                                 << auxbasis.AOBasisSize() << flush;

  /*
   * for the representation of 2-point functions with the help of the
   * auxiliary basis, its AO overlap matrix is required.
   * cf. M. Rohlfing, PhD thesis, ch. 3
   */
  AOOverlap _auxoverlap;
  // Fill overlap
  _auxoverlap.Fill(auxbasis);

  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " Filled Aux Overlap matrix of dimension: "
                                 << _auxoverlap.Matrix().rows() << flush;

  /*
   *  for the calculation of Coulomb and exchange term in the self
   *  energy and electron-hole interaction, the Coulomb interaction
   *  is represented using the auxiliary basis set.
   *  Here, we need to prepare the Coulomb matrix expressed in
   *  the AOs of the auxbasis
   */

  // get Coulomb matrix as AOCoulomb
  AOCoulomb _auxcoulomb;

  // Fill Coulomb matrix
  _auxcoulomb.Fill(auxbasis);
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " Filled Aux Coulomb matrix of dimension: "
                                 << _auxcoulomb.Matrix().rows() << flush;
 

  Eigen::MatrixXd Coulomb_sqrtInv=_auxcoulomb.Pseudo_InvSqrt_GWBSE(_auxoverlap,1e-8);
    _auxoverlap.FreeMatrix();
    _auxcoulomb.FreeMatrix();
  CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp() << " Calculated Matrix Sqrt of Aux Coulomb Matrix"
      << flush;
  CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp() << " Removed " << _auxcoulomb.Removedfunctions()
      << " functions from Aux Coulomb matrix to avoid near linear dependencies" << flush;
  
  // container => M_mn
  // prepare 3-center integral object

  TCMatrix_gwbse Mmn;
  //rpamin here, because RPA needs till rpamin
  Mmn.Initialize(auxbasis.AOBasisSize(), _rpamin, _qpmax, _rpamin, _rpamax);
  Mmn.Fill(auxbasis, dftbasis, _orbitals.MOCoefficients());
  CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp()
      << " Calculated Mmn_beta (3-center-repulsion x orbitals)  " << flush;

  // make _Mmn symmetric
  Mmn.MultiplyRightWithAuxMatrix(Coulomb_sqrtInv);
  
  CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp()
      << " Multiplied Mmn_beta with Coulomb Matrix " << flush;
  PPM ppm;
  RPA rpa;
  rpa.configure(_homo,_rpamin,_rpamax);
  Eigen::VectorXd screen_r=Eigen::VectorXd::Zero(1);
  screen_r(0)=ppm.getScreening_r();
  Eigen::VectorXd screen_i=Eigen::VectorXd::Zero(1);
  screen_i(0)=ppm.getScreening_i();
  rpa.setScreening(screen_r,screen_i);
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " Prepared RPA  " << flush;
  
  Sigma sigma=Sigma(_pLog);
  sigma.configure(_homo,_qpmin,_qpmax,_g_sc_max_iterations,_g_sc_limit);
  sigma.setDFTdata(_orbitals.getScaHFX(),&vxc,&_orbitals.MOEnergies());
 
  // initialize _qp_energies;
  // shift unoccupied levels by the shift
  Eigen::VectorXd gwa_energies = Eigen::VectorXd::Zero(_orbitals.getNumberOfLevels());
  for (int i = 0; i < gwa_energies.size(); ++i) {
    gwa_energies(i) = _orbitals.MOEnergies()(i);
    if (i > int(_homo)) {
      gwa_energies(i) += _shift;
    }
  }
  sigma.setGWAEnergies(gwa_energies);
  
   /* for automatic iteration of both G and W, we need to
   * - make a copy of _Mmn
   * - calculate eps
   * - construct ppm
   * - threecenters for sigma
   * - sigma_x
   * - sigma_c
   * - test for convergence
   *
   */

  if (!_iterate_gw) {
    _gw_sc_max_iterations = 1;
  }

  const Eigen::VectorXd &_dft_energies = _orbitals.MOEnergies();
  for (unsigned gw_iteration = 0; gw_iteration < _gw_sc_max_iterations;
       ++gw_iteration) {

    Eigen::VectorXd _qp_old_rpa = gwa_energies;
    if (_iterate_gw) {
      CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " GW Iteraton "
                                     << gw_iteration + 1 << " of "
                                     << _gw_sc_max_iterations << flush;
    }
    
    
    if(gw_iteration%_reset_3c==0 && gw_iteration!=0){
      CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp() << " Rebuilding Mmn_beta (3-center-repulsion x orbitals)" << flush;
       Mmn.Fill(auxbasis, dftbasis, _orbitals.MOCoefficients());
       Mmn.MultiplyRightWithAuxMatrix(Coulomb_sqrtInv);
    }
  
    rpa.calculate_epsilon(gwa_energies,Mmn);
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                   << " Calculated epsilon via RPA  " << flush;
    ppm.PPM_construct_parameters(rpa);
    CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                   << " Constructed PPM parameters  " << flush;
    
    
    
    Mmn.MultiplyRightWithAuxMatrix(ppm.getPpm_phi());
    CTP_LOG(ctp::logDEBUG, *_pLog)
        << ctp::TimeStamp() << " Prepared threecenters for sigma  " << flush;

    sigma.CalcdiagElements(Mmn,ppm);
    CTP_LOG(ctp::logDEBUG, *_pLog)
        << ctp::TimeStamp() << " Calculated diagonal part of Sigma  " << flush;
    // iterative refinement of qp energies
    gwa_energies=sigma.getGWAEnergies();
    double _DFTgap = _dft_energies(_homo + 1) - _dft_energies(_homo);
    double _QPgap = gwa_energies(_homo + 1) - gwa_energies(_homo);
    _shift = _QPgap - _DFTgap;
    
    // qp energies outside the update range are simply shifted.
    for (unsigned i = _qpmax + 1; i < _dft_energies.size(); ++i) {
      gwa_energies(i) = _dft_energies(i) + _shift;
    }
    
    if (_iterate_gw) {
      bool _gw_converged = true;
      Eigen::VectorXd diff = _qp_old_rpa - gwa_energies;
      int state = 0;
      double E_diff_max=diff.cwiseAbs().maxCoeff(&state);
      if(E_diff_max>_gw_sc_limit){
           _gw_converged = false;
      }
      
      double alpha = 0.0;
      gwa_energies = alpha * _qp_old_rpa + (1 - alpha) * gwa_energies;
      sigma.setGWAEnergies(gwa_energies);
      if (tools::globals::verbose) {
        CTP_LOG(ctp::logDEBUG, *_pLog)
            << ctp::TimeStamp() << " GW_Iteration: " << gw_iteration + 1
            << " shift=" << _shift << " E_diff max=" << E_diff_max
            << " StateNo:" << state << flush;
      }

      if (_gw_converged) {
        CTP_LOG(ctp::logDEBUG, *_pLog)
            << ctp::TimeStamp() << " Converged after " << gw_iteration + 1
            << " GW iterations" << flush;
        break;
      } else if (gw_iteration == _gw_sc_max_iterations - 1) {
        // continue regardless for now, but drop WARNING
        CTP_LOG(ctp::logDEBUG, *_pLog)
            << ctp::TimeStamp() << " WARNING! GWA spectrum not converged after "
            << _gw_sc_max_iterations << " iterations." << flush;
        CTP_LOG(ctp::logDEBUG, *_pLog)
            << ctp::TimeStamp() << "          GWA level " << state
            << " energy changed by " << E_diff_max << flush;
        CTP_LOG(ctp::logDEBUG, *_pLog)
            << ctp::TimeStamp()
            << "          Run continues. Inspect results carefully!" << flush;
        break;
      }
    }else
    {
      sigma.setGWAEnergies(gwa_energies);
     }
  }
   
  ppm.FreeMatrix();
  rpa.FreeMatrices();
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                   << " Cleaned up PPM and MmnRPA Matrices" << flush;
  

  // Output of quasiparticle energies after all is done:
  PrintGWA_Energies(vxc, sigma, _dft_energies);

  // store perturbative QP energy data in orbitals object (DFT, S_x,S_c, V_xc,
  // E_qp)
  if (_store_qp_pert) {
    Eigen::MatrixXd &_qp_energies_store = _orbitals.QPpertEnergies();
    _qp_energies_store=Eigen::MatrixXd::Zero(_qptotal, 5);
    for (unsigned i = 0; i < _qptotal; i++) {
      _qp_energies_store(i, 0) = _dft_energies(i + _qpmin);
      _qp_energies_store(i, 1) = sigma.x(i);
      _qp_energies_store(i, 2) = sigma.c(i);
      _qp_energies_store(i, 3) = vxc(i, i);
      _qp_energies_store(i, 4) = gwa_energies(i + _qpmin);
    }
  }
 

  
  if (_do_qp_diag || _do_bse_singlets || _do_bse_triplets) {
      CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp() << " Calculating offdiagonal part of Sigma  " << flush;
  sigma.CalcOffDiagElements(Mmn,ppm);
   CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp() << " Calculated offdiagonal part of Sigma  " << flush;
    // free no longer required three-center matrices in _Mmn
  // max required is _bse_cmax (could be smaller than _qpmax)
  Mmn.Prune(_bse_vmin, _bse_cmax);   
    Eigen::MatrixXd Hqp=sigma.SetupFullQPHamiltonian();
 
    if (_do_qp_diag) {
      Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(Hqp);
      const Eigen::VectorXd& qp_diag_energies=es.eigenvalues();
    
      if(es.info()==Eigen::ComputationInfo::Success){
    CTP_LOG(ctp::logDEBUG, *_pLog)
      << ctp::TimeStamp() << " Diagonalized QP Hamiltonian  "<< flush;
      }
      
      PrintQP_Energies(gwa_energies, qp_diag_energies);

      if (_store_qp_diag) {
        _orbitals.QPdiagCoefficients()=es.eigenvectors();
        _orbitals.QPdiagEnergies()=es.eigenvalues();
      }
    }  // _do_qp_diag
  

  // proceed only if BSE requested
  if (_do_bse_singlets || _do_bse_triplets) {
      
      BSE bse=BSE(_orbitals,_pLog,_min_print_weight);
      bse.setBSEindices(_homo,_bse_vmin,_bse_cmax,_bse_maxeigenvectors);
      bse.setGWData(&Mmn,&ppm,&Hqp);
       // calculate direct part of eh interaction, needed for singlets and triplets
    
      
    


        if (_do_bse_triplets && _do_bse_diag) {
            bse.Solve_triplets();
            CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                    << " Solved BSE for triplets " << flush;
            // analyze and report results
            bse.Analyze_triplets(dftbasis);

            if (!_store_bse_triplets) {
                bse.FreeTriplets();
            }

        } // do_triplets

        if (_do_bse_singlets && _do_bse_diag) {

            if (_do_full_BSE) {
                bse.Solve_singlets_BTDA();
                CTP_LOG(ctp::logDEBUG, *_pLog)
                        << ctp::TimeStamp() << " Solved full BSE for singlets " << flush;
            } else {
                bse.Solve_singlets();
                CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                        << " Solved BSE for singlets " << flush;
            }
            bse.Analyze_singlets(dftbasis);
            if (!_store_bse_singlets) {
                bse.FreeSinglets();
            }
        }

        if (_store_eh_interaction) {
            if (_do_bse_singlets) {
                bse.SetupHs();
            }
            if (_do_bse_triplets) {
                bse.SetupHt();
            }
        }
    }
  }
  CTP_LOG(ctp::logDEBUG, *_pLog) << ctp::TimeStamp()
                                 << " GWBSE calculation finished " << flush;
  return true;
}
}
};
