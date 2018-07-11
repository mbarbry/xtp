/*
 * Copyright 2009-2018 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE threecenter_test
#include <boost/test/unit_test.hpp>
#include <votca/xtp/orbitals.h>
#include <votca/xtp/threecenter.h>
#include <votca/xtp/ERIs.h>

using namespace votca::xtp;
using namespace std;

BOOST_AUTO_TEST_SUITE(threecenter_test)

BOOST_AUTO_TEST_CASE(threecenter_dft) {
  
  ofstream xyzfile("molecule.xyz");
  xyzfile << " 5" << endl;
  xyzfile << " methane" << endl;
  xyzfile << " C            .000000     .000000     .000000" << endl;
  xyzfile << " H            .629118     .629118     .629118" << endl;
  xyzfile << " H           -.629118    -.629118     .629118" << endl;
  xyzfile << " H            .629118    -.629118    -.629118" << endl;
  xyzfile << " H           -.629118     .629118    -.629118" << endl;
  xyzfile.close();

  ofstream basisfile("3-21G.xml");
  basisfile <<"<basis name=\"3-21G\">" << endl;
  basisfile << "  <element name=\"H\">" << endl;
  basisfile << "    <shell scale=\"1.0\" type=\"S\">" << endl;
  basisfile << "      <constant decay=\"5.447178e+00\">" << endl;
  basisfile << "        <contractions factor=\"1.562850e-01\" type=\"S\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "      <constant decay=\"8.245470e-01\">" << endl;
  basisfile << "        <contractions factor=\"9.046910e-01\" type=\"S\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "    </shell>" << endl;
  basisfile << "    <shell scale=\"1.0\" type=\"S\">" << endl;
  basisfile << "      <constant decay=\"1.831920e-01\">" << endl;
  basisfile << "        <contractions factor=\"1.000000e+00\" type=\"S\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "    </shell>" << endl;
  basisfile << "  </element>" << endl;
  basisfile << "  <element name=\"C\">" << endl;
  basisfile << "    <shell scale=\"1.0\" type=\"S\">" << endl;
  basisfile << "      <constant decay=\"1.722560e+02\">" << endl;
  basisfile << "        <contractions factor=\"6.176690e-02\" type=\"S\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "      <constant decay=\"2.591090e+01\">" << endl;
  basisfile << "        <contractions factor=\"3.587940e-01\" type=\"S\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "      <constant decay=\"5.533350e+00\">" << endl;
  basisfile << "        <contractions factor=\"7.007130e-01\" type=\"S\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "    </shell>" << endl;
  basisfile << "    <shell scale=\"1.0\" type=\"SP\">" << endl;
  basisfile << "      <constant decay=\"3.664980e+00\">" << endl;
  basisfile << "        <contractions factor=\"-3.958970e-01\" type=\"S\"/>" << endl;
  basisfile << "        <contractions factor=\"2.364600e-01\" type=\"P\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "      <constant decay=\"7.705450e-01\">" << endl;
  basisfile << "        <contractions factor=\"1.215840e+00\" type=\"S\"/>" << endl;
  basisfile << "        <contractions factor=\"8.606190e-01\" type=\"P\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "    </shell>" << endl;
  basisfile << "    <shell scale=\"1.0\" type=\"SP\">" << endl;
  basisfile << "      <constant decay=\"1.958570e-01\">" << endl;
  basisfile << "        <contractions factor=\"1.000000e+00\" type=\"S\"/>" << endl;
  basisfile << "        <contractions factor=\"1.000000e+00\" type=\"P\"/>" << endl;
  basisfile << "      </constant>" << endl;
  basisfile << "    </shell>" << endl;
  basisfile << "  </element>" << endl;
  basisfile << "</basis>" << endl;
  basisfile.close();
  
  Orbitals orbitals;
  orbitals.LoadFromXYZ("molecule.xyz");
  BasisSet basis;
  basis.LoadBasisSet("3-21G.xml");
  AOBasis aobasis;
  aobasis.AOBasisFill(&basis,orbitals.QMAtoms());
  TCMatrix_dft threec;
  threec.Fill(aobasis,aobasis,Eigen::MatrixXd::Identity(aobasis.AOBasisSize(),aobasis.AOBasisSize()));
  
  
  
  
  Eigen::MatrixXd Res0=Eigen::MatrixXd::Zero(aobasis.AOBasisSize(),aobasis.AOBasisSize());
  threec.getDatamatrix(0).AddtoEigenMatrix(Res0);

  
  Eigen::MatrixXd Res4=Eigen::MatrixXd::Zero(aobasis.AOBasisSize(),aobasis.AOBasisSize());
  threec.getDatamatrix(4).AddtoEigenMatrix(Res4);

  
  Eigen::MatrixXd Ref0=Eigen::MatrixXd::Zero(aobasis.AOBasisSize(),aobasis.AOBasisSize());
  Ref0<<2.29374 ,0.304177 ,0 ,0 ,0 ,0.337068 ,0 ,0 ,0 ,0.033705 ,0.150367 ,0.033705 ,0.150367 ,0.033705 ,0.150367 ,0.033705 ,0.150367,
0.304177 ,0.921384 ,0 ,0 ,0 ,0.632988 ,0 ,0 ,0 ,0.138532 ,0.319933 ,0.138532 ,0.319933 ,0.138532 ,0.319933 ,0.138532 ,0.319933,
0 ,0 ,0.967997 ,0 ,0 ,0 ,0.366964 ,0 ,0 ,0.109996 ,0.0905438 ,0.109996 ,0.0905438 ,-0.109996 ,-0.0905438 ,-0.109996 ,-0.0905438,
0 ,0 ,0 ,0.967997 ,0 ,0 ,0 ,0.366964 ,0 ,0.109996 ,0.0905438 ,-0.109996 ,-0.0905438 ,-0.109996 ,-0.0905438 ,0.109996 ,0.0905438,
0 ,0 ,0 ,0 ,0.967997 ,0 ,0 ,0 ,0.366964 ,0.109996 ,0.0905438 ,-0.109996 ,-0.0905438 ,0.109996 ,0.0905438 ,-0.109996 ,-0.0905438,
0.337068 ,0.632988 ,0 ,0 ,0 ,0.616586 ,0 ,0 ,0 ,0.17704 ,0.362827 ,0.17704 ,0.362827 ,0.17704 ,0.362827 ,0.17704 ,0.362827,
0 ,0 ,0.366964 ,0 ,0 ,0 ,0.423084 ,0 ,0 ,0.130034 ,0.131792 ,0.130034 ,0.131792 ,-0.130034 ,-0.131792 ,-0.130034 ,-0.131792,
0 ,0 ,0 ,0.366964 ,0 ,0 ,0 ,0.423084 ,0 ,0.130034 ,0.131792 ,-0.130034 ,-0.131792 ,-0.130034 ,-0.131792 ,0.130034 ,0.131792,
0 ,0 ,0 ,0 ,0.366964 ,0 ,0 ,0 ,0.423084 ,0.130034 ,0.131792 ,-0.130034 ,-0.131792 ,0.130034 ,0.131792 ,-0.130034 ,-0.131792,
0.033705 ,0.138532 ,0.109996 ,0.109996 ,0.109996 ,0.17704 ,0.130034 ,0.130034 ,0.130034 ,0.436704 ,0.280477 ,0.00555884 ,0.0631251 ,0.00555884 ,0.0631251 ,0.00555884 ,0.0631251,
0.150367 ,0.319933 ,0.0905438 ,0.0905438 ,0.0905438 ,0.362827 ,0.131792 ,0.131792 ,0.131792 ,0.280477 ,0.39915 ,0.0631251 ,0.182028 ,0.0631251 ,0.182028 ,0.0631251 ,0.182028,
0.033705 ,0.138532 ,0.109996 ,-0.109996 ,-0.109996 ,0.17704 ,0.130034 ,-0.130034 ,-0.130034 ,0.00555884 ,0.0631251 ,0.436704 ,0.280477 ,0.00555884 ,0.0631251 ,0.00555884 ,0.0631251,
0.150367 ,0.319933 ,0.0905438 ,-0.0905438 ,-0.0905438 ,0.362827 ,0.131792 ,-0.131792 ,-0.131792 ,0.0631251 ,0.182028 ,0.280477 ,0.39915 ,0.0631251 ,0.182028 ,0.0631251 ,0.182028,
0.033705 ,0.138532 ,-0.109996 ,-0.109996 ,0.109996 ,0.17704 ,-0.130034 ,-0.130034 ,0.130034 ,0.00555884 ,0.0631251 ,0.00555884 ,0.0631251 ,0.436704 ,0.280477 ,0.00555884 ,0.0631251,
0.150367 ,0.319933 ,-0.0905438 ,-0.0905438 ,0.0905438 ,0.362827 ,-0.131792 ,-0.131792 ,0.131792 ,0.0631251 ,0.182028 ,0.0631251 ,0.182028 ,0.280477 ,0.39915 ,0.0631251 ,0.182028,
0.033705 ,0.138532 ,-0.109996 ,0.109996 ,-0.109996 ,0.17704 ,-0.130034 ,0.130034 ,-0.130034 ,0.00555884 ,0.0631251 ,0.00555884 ,0.0631251 ,0.00555884 ,0.0631251 ,0.436704 ,0.280477,
0.150367 ,0.319933 ,-0.0905438 ,0.0905438 ,-0.0905438 ,0.362827 ,-0.131792 ,0.131792 ,-0.131792 ,0.0631251 ,0.182028 ,0.0631251 ,0.182028 ,0.0631251 ,0.182028 ,0.280477 ,0.39915;
  
  Eigen::MatrixXd Ref4=Eigen::MatrixXd::Zero(aobasis.AOBasisSize(),aobasis.AOBasisSize());
  Ref4<<0 ,0 ,0 ,0 ,0.21851 ,0 ,0 ,0 ,0.0305022 ,0.00646054 ,0.00674129 ,-0.00646054 ,-0.00674129 ,0.00646054 ,0.00674129 ,-0.00646054 ,-0.00674129,
0 ,0 ,0 ,0 ,0.896593 ,0 ,0 ,0 ,0.409428 ,0.125791 ,0.10102 ,-0.125791 ,-0.10102 ,0.125791 ,0.10102 ,-0.125791 ,-0.10102,
0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.0866627 ,0.0226036 ,-0.0866627 ,-0.0226036 ,-0.0866627 ,-0.0226036 ,0.0866627 ,0.0226036,
0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.0866627 ,0.0226036 ,0.0866627 ,0.0226036 ,-0.0866627 ,-0.0226036 ,-0.0866627 ,-0.0226036,
0.21851 ,0.896593 ,0 ,0 ,0 ,0.690122 ,0 ,0 ,0 ,0.17602 ,0.363256 ,0.17602 ,0.363256 ,0.17602 ,0.363256 ,0.17602 ,0.363256,
0 ,0 ,0 ,0 ,0.690122 ,0 ,0 ,0 ,0.56523 ,0.18263 ,0.159752 ,-0.18263 ,-0.159752 ,0.18263 ,0.159752 ,-0.18263 ,-0.159752,
0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.122829 ,0.0489282 ,-0.122829 ,-0.0489282 ,-0.122829 ,-0.0489282 ,0.122829 ,0.0489282,
0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0.122829 ,0.0489282 ,0.122829 ,0.0489282 ,-0.122829 ,-0.0489282 ,-0.122829 ,-0.0489282,
0.0305022 ,0.409428 ,0 ,0 ,0 ,0.56523 ,0 ,0 ,0 ,0.201612 ,0.37355 ,0.201612 ,0.37355 ,0.201612 ,0.37355 ,0.201612 ,0.37355,
0.00646054 ,0.125791 ,0.0866627 ,0.0866627 ,0.17602 ,0.18263 ,0.122829 ,0.122829 ,0.201612 ,0.540893 ,0.311822 ,2.46557e-19 ,0.0523107 ,0.00882417 ,0.0808349 ,2.44863e-19 ,0.0523107,
0.00674129 ,0.10102 ,0.0226036 ,0.0226036 ,0.363256 ,0.159752 ,0.0489282 ,0.0489282 ,0.37355 ,0.311822 ,0.304029 ,-0.0523107 ,3.0974e-18 ,0.0808349 ,0.15978 ,-0.0523107 ,3.0974e-18,
-0.00646054 ,-0.125791 ,-0.0866627 ,0.0866627 ,0.17602 ,-0.18263 ,-0.122829 ,0.122829 ,0.201612 ,2.46557e-19 ,-0.0523107 ,-0.540893 ,-0.311822 ,-2.44863e-19 ,-0.0523107 ,-0.00882417 ,-0.0808349,
-0.00674129 ,-0.10102 ,-0.0226036 ,0.0226036 ,0.363256 ,-0.159752 ,-0.0489282 ,0.0489282 ,0.37355 ,0.0523107 ,3.0974e-18 ,-0.311822 ,-0.304029 ,0.0523107 ,-3.0974e-18 ,-0.0808349 ,-0.15978,
0.00646054 ,0.125791 ,-0.0866627 ,-0.0866627 ,0.17602 ,0.18263 ,-0.122829 ,-0.122829 ,0.201612 ,0.00882417 ,0.0808349 ,-2.44863e-19 ,0.0523107 ,0.540893 ,0.311822 ,2.46557e-19 ,0.0523107,
0.00674129 ,0.10102 ,-0.0226036 ,-0.0226036 ,0.363256 ,0.159752 ,-0.0489282 ,-0.0489282 ,0.37355 ,0.0808349 ,0.15978 ,-0.0523107 ,-3.0974e-18 ,0.311822 ,0.304029 ,-0.0523107 ,3.0974e-18,
-0.00646054 ,-0.125791 ,0.0866627 ,-0.0866627 ,0.17602 ,-0.18263 ,0.122829 ,-0.122829 ,0.201612 ,2.44863e-19 ,-0.0523107 ,-0.00882417 ,-0.0808349 ,2.46557e-19 ,-0.0523107 ,-0.540893 ,-0.311822,
-0.00674129 ,-0.10102 ,0.0226036 ,-0.0226036 ,0.363256 ,-0.159752 ,0.0489282 ,-0.0489282 ,0.37355 ,0.0523107 ,3.0974e-18 ,-0.0808349 ,-0.15978 ,0.0523107 ,3.0974e-18 ,-0.311822 ,-0.304029;
  
  bool check_threec=(Res0.isApprox(Ref0,0.00001) && Res4.isApprox(Ref4,0.00001) );
BOOST_CHECK_EQUAL(check_threec, 1);

}

BOOST_AUTO_TEST_CASE(threecenter_gwbse){
  Orbitals orbitals;
  orbitals.LoadFromXYZ("molecule.xyz");
  BasisSet basis;
  basis.LoadBasisSet("3-21G.xml");
  AOBasis aobasis;
  aobasis.AOBasisFill(&basis,orbitals.QMAtoms());
  
Eigen::MatrixXd MOs=Eigen::MatrixXd::Ones(17,17);
MOs<<-0.00761992, -4.69664e-13, 8.35009e-15, -1.15214e-14, -0.0156169, -2.23157e-12, 1.52916e-14, 2.10997e-15, 8.21478e-15, 3.18517e-15, 2.89043e-13, -0.00949189, 1.95787e-12, 1.22168e-14, -2.63092e-15, -0.22227, 1.00844,
0.233602, -3.18103e-12, 4.05093e-14, -4.70943e-14, 0.1578, 4.75897e-11, -1.87447e-13, -1.02418e-14, 6.44484e-14, -2.6602e-14, 6.5906e-12, -0.281033, -6.67755e-12, 2.70339e-14, -9.78783e-14, -1.94373, -0.36629,
-1.63678e-13, -0.22745, -0.054851, 0.30351, 3.78688e-11, -0.201627, -0.158318, -0.233561, -0.0509347, -0.650424, 0.452606, -5.88565e-11, 0.453936, -0.165715, -0.619056, 7.0149e-12, 2.395e-14,
-4.51653e-14, -0.216509, 0.296975, -0.108582, 3.79159e-11, -0.199301, 0.283114, -0.0198557, 0.584622, 0.275311, 0.461431, -5.93732e-11, 0.453057, 0.619523, 0.166374, 7.13235e-12, 2.56811e-14,
-9.0903e-14, -0.21966, -0.235919, -0.207249, 3.75979e-11, -0.199736, -0.122681, 0.255585, -0.534902, 0.362837, 0.461224, -5.91028e-11, 0.453245, -0.453298, 0.453695, 7.01644e-12, 2.60987e-14,
0.480866, 1.8992e-11, -2.56795e-13, 4.14571e-13, 2.2709, 4.78615e-10, -2.39153e-12, -2.53852e-13, -2.15605e-13, -2.80359e-13, 7.00137e-12, 0.145171, -1.96136e-11, -2.24876e-13, -2.57294e-14, 4.04176, 0.193617,
-1.64421e-12, -0.182159, -0.0439288, 0.243073, 1.80753e-10, -0.764779, -0.600505, -0.885907, 0.0862014, 1.10077, -0.765985, 6.65828e-11, -0.579266, 0.211468, 0.789976, -1.41532e-11, -1.29659e-13,
-1.64105e-12, -0.173397, 0.23784, -0.0869607, 1.80537e-10, -0.755957, 1.07386, -0.0753135, -0.989408, -0.465933, -0.78092, 6.72256e-11, -0.578145, -0.790571, -0.212309, -1.42443e-11, -1.31306e-13,
-1.63849e-12, -0.17592, -0.188941, -0.165981, 1.79403e-10, -0.757606, -0.465334, 0.969444, 0.905262, -0.61406, -0.78057, 6.69453e-11, -0.578385, 0.578453, -0.578959, -1.40917e-11, -1.31002e-13,
0.129798, -0.274485, 0.00256652, -0.00509635, -0.0118465, 0.141392, -0.000497905, -0.000510338, -0.000526798, -0.00532572, 0.596595, 0.65313, -0.964582, -0.000361559, -0.000717866, -0.195084, 0.0246232,
0.0541331, -0.255228, 0.00238646, -0.0047388, -0.88576, 1.68364, -0.00592888, -0.00607692, -9.5047e-05, -0.000960887, 0.10764, -0.362701, 1.53456, 0.000575205, 0.00114206, -0.793844, -0.035336,
0.129798, 0.0863299, -0.0479412, 0.25617, -0.0118465, -0.0464689, 0.0750316, 0.110468, -0.0436647, -0.558989, -0.203909, 0.65313, 0.320785, 0.235387, 0.878697, -0.195084, 0.0246232,
0.0541331, 0.0802732, -0.0445777, 0.238198, -0.88576, -0.553335, 0.893449, 1.31541, -0.00787816, -0.100855, -0.0367902, -0.362701, -0.510338, -0.374479, -1.39792, -0.793844, -0.035336,
0.129798, 0.0927742, -0.197727, -0.166347, -0.0118465, -0.0473592, 0.0582544, -0.119815, -0.463559, 0.320126, -0.196433, 0.65313, 0.321765, 0.643254, -0.642737, -0.195084, 0.0246232,
0.0541331, 0.0862654, -0.183855, -0.154677, -0.88576, -0.563936, 0.693672, -1.42672, -0.0836372, 0.0577585, -0.0354411, -0.362701, -0.511897, -1.02335, 1.02253, -0.793844, -0.035336,
0.129798, 0.0953806, 0.243102, -0.0847266, -0.0118465, -0.0475639, -0.132788, 0.00985812, 0.507751, 0.244188, -0.196253, 0.65313, 0.322032, -0.87828, -0.235242, -0.195084, 0.0246232,
0.0541331, 0.088689, 0.226046, -0.0787824, -0.88576, -0.566373, -1.58119, 0.117387, 0.0916104, 0.0440574, -0.0354087, -0.362701, -0.512321, 1.39726, 0.374248, -0.793844, -0.035336;


TCMatrix_gwbse tc;
tc.Initialize(aobasis.AOBasisSize(),0,5,0,7);
tc.Fill(aobasis,aobasis,MOs);

MatrixXfd ref0b=MatrixXfd::Zero(8,17);
ref0b<<0.628851,3.03881,-2.05006e-12,-1.87836e-12,-1.94044e-12,5.5503,-4.13501e-12,-3.90883e-12,-3.9873e-12,1.87792,4.83175,1.87792,4.83175,1.87792,4.83175,1.87792,4.83175,
-2.04532e-07,-1.04459e-06,-0.400163,-0.380915,-0.386458,-2.01058e-06,-0.731793,-0.696592,-0.706729,-0.711232,-0.863134,0.223693,0.271467,0.240391,0.291732,0.247145,0.299928,
-2.19659e-08,-9.08077e-08,-0.0965019,0.522482,-0.415063,-1.5839e-07,-0.176476,0.95548,-0.759039,0.00665039,0.00807071,-0.124223,-0.150754,-0.51234,-0.621763,0.629913,0.764446,
-6.19672e-08,-3.07929e-07,0.533978,-0.191033,-0.364623,-5.8613e-07,0.976505,-0.349349,-0.666798,-0.0132057,-0.0160265,0.663775,0.805541,-0.431031,-0.523089,-0.219539,-0.266428,
0.204367,0.697838,1.26699e-10,1.26661e-10,1.2579e-10,0.692773,2.4627e-10,2.4611e-10,2.44474e-10,0.185357,0.396119,0.185357,0.396119,0.185357,0.396119,0.185357,0.396119,
-1.44054e-06,-7.22902e-06,-0.156777,-0.154968,-0.155307,-1.38159e-05,-0.122748,-0.121331,-0.121596,0.0511716,0.0213704,-0.016824,-0.0070399,-0.0171462,-0.00717437,-0.01722,-0.00720508,
7.82802e-07,3.93403e-06,-0.123101,0.220136,-0.0953918,7.52301e-06,-0.096381,0.172351,-0.0746861,-0.000178717,-7.00339e-05,0.0271613,0.0113555,0.0210887,0.00881809,-0.0480611,-0.0200769,
1.86405e-07,9.82883e-07,-0.181606,-0.0154391,0.198731,1.91547e-06,-0.142185,-0.012088,0.155593,-0.000184107,-7.55347e-05,0.0399864,0.0167108,-0.0433682,-0.0181208,0.00356867,0.00149244;

bool check0_before=ref0b.isApprox(tc[0],1e-5);
if(!check0_before){
 cout<<"tc0"<<endl;
   cout<<tc[0]<<endl;
    cout<<"tc0_ref"<<endl;
   cout<<ref0b<<endl;
 }
 BOOST_CHECK_EQUAL(check0_before , true);
 
 
MatrixXfd ref2b=MatrixXfd::Zero(8,17);
ref2b<<-2.19659e-08,-9.08077e-08,-0.0965019,0.522482,-0.415063,-1.5839e-07,-0.176476,0.95548,-0.759039,0.00665039,0.00807071,-0.124223,-0.150754,-0.51234,-0.621763,0.629913,0.764446,
-1.89528e-07,-1.11342e-06,-0.0218804,0.101571,-0.0860557,-2.3609e-06,-0.0534718,0.248221,-0.210305,-0.00848538,-0.00850501,-0.049847,-0.049956,-0.220931,-0.221409,0.27926,0.279862,
0.563821,2.79807,-0.216602,0.0400061,-0.0503598,5.31317,-0.529336,0.0977678,-0.12307,1.52585,4.39618,1.55345,4.42385,1.99663,4.86798,2.23754,5.10941,
-5.47528e-08,-3.15719e-07,-0.0555419,-0.0931117,0.148535,-6.32992e-07,-0.135734,-0.227548,0.362994,-0.000157694,-0.000158364,-0.147911,-0.148231,0.396136,0.39699,-0.248068,-0.248603,
-6.43711e-09,-5.35154e-09,-0.0189872,0.1028,-0.0816654,5.26219e-08,0.0326842,-0.17696,0.140578,-0.001343,-0.00271152,0.0250853,0.050648,0.103461,0.20889,-0.127203,-0.256826,
-0.00126244,-0.00443026,0.0198805,-0.0946089,0.0791267,-0.00464413,0.0605858,-0.288321,0.241139,0.00306364,0.00354726,0.0317403,0.0471204,0.141478,0.213863,-0.18032,-0.275099,
0.152659,0.535732,0.166872,-0.0712572,0.10111,0.561641,0.508545,-0.217158,0.308133,0.304304,0.596376,0.249011,0.512359,0.127203,0.327277,-0.192062,-0.157836,
-0.0669603,-0.234987,-0.130276,-0.0664132,0.110369,-0.246351,-0.397017,-0.202395,0.336351,-0.133505,-0.261631,-0.214922,-0.38534,0.23081,0.291934,-0.0966333,-0.205605;

bool check2_before=ref2b.isApprox(tc[2],1e-5);
if(!check2_before){
 cout<<"tc2"<<endl;
   cout<<tc[2]<<endl;
    cout<<"tc2_ref"<<endl;
   cout<<ref2b<<endl;
 }

BOOST_CHECK_EQUAL(check2_before , true);
 

MatrixXfd ref4b=MatrixXfd::Zero(8,17);
ref4b<<0.204367,0.697838,1.26699e-10,1.26661e-10,1.2579e-10,0.692773,2.4627e-10,2.4611e-10,2.44474e-10,0.185357,0.396119,0.185357,0.396119,0.185357,0.396119,0.185357,0.396119,
-1.24257e-08,4.23022e-08,-0.0787338,-0.0749464,-0.0760372,3.42504e-07,0.135532,0.129012,0.13089,0.143624,0.289982,-0.045172,-0.0912034,-0.0485441,-0.0980116,-0.0499077,-0.100765,
-6.43711e-09,-5.35154e-09,-0.0189872,0.1028,-0.0816654,5.26219e-08,0.0326842,-0.17696,0.140578,-0.001343,-0.00271152,0.0250853,0.050648,0.103461,0.20889,-0.127203,-0.256826,
-5.76443e-09,9.06836e-09,0.105063,-0.0375866,-0.071741,1.10508e-07,-0.180853,0.0647012,0.123494,0.00266682,0.00538441,-0.134041,-0.270632,0.0870411,0.175739,0.0443331,0.0895098,
0.417824,2.04165,3.63427e-11,3.63751e-11,3.60811e-11,4.23787,-1.22792e-10,-1.22567e-10,-1.21864e-10,1.42386,3.98676,1.42386,3.98676,1.42386,3.98676,1.42386,3.98676,
-1.17012e-07,2.44306e-07,-0.176749,-0.174711,-0.175092,2.51858e-06,-0.418457,-0.41363,-0.414532,-0.434977,-0.697424,0.142958,0.229215,0.145697,0.233606,0.146326,0.234615,
6.2215e-08,-1.35364e-07,-0.138784,0.248182,-0.107544,-1.36408e-06,-0.328573,0.587577,-0.254613,0.00153164,0.00245511,-0.230828,-0.370102,-0.179215,-0.287347,0.408509,0.654987,
3.76948e-09,-5.37914e-08,-0.204744,-0.0174058,0.22405,-2.9111e-07,-0.484736,-0.0412086,0.530444,0.00156996,0.00251706,-0.339844,-0.544893,0.368601,0.591001,-0.0303277,-0.0486264;

bool check4_before=ref4b.isApprox(tc[4],1e-5);
if(!check4_before){
 cout<<"tc4"<<endl;
   cout<<tc[4]<<endl;
    cout<<"tc4_ref"<<endl;
   cout<<ref4b<<endl;
 }

BOOST_CHECK_EQUAL(check4_before , true);



AOOverlap overlap;
overlap.Fill(aobasis);

AOCoulomb cou;
cou.Fill(aobasis);

tc.MultiplyRightWithAuxMatrix(cou.Pseudo_InvSqrt_GWBSE(overlap,1e-7));

MatrixXfd ref0=MatrixXfd::Zero(8,17);
ref0<<0.0999942,0.397538,-6.47732e-13,-5.76178e-13,-6.0385e-13,0.346631,-5.81868e-13,-5.60885e-13,-5.62217e-13,0.146983,0.189933,0.146983,0.189933,0.146983,0.189933,0.146983,0.189933,
-2.44113e-08,-1.15535e-07,-0.132577,-0.1262,-0.128036,-1.25427e-07,-0.0826604,-0.0786842,-0.0798293,-0.158432,-0.0698594,0.0498293,0.0219718,0.0535489,0.023612,0.0550533,0.0242753,
-5.051e-09,-1.41029e-08,-0.0319717,0.173102,-0.137513,-1.56769e-08,-0.0199341,0.107927,-0.085738,0.00148142,0.000653224,-0.0276715,-0.0122016,-0.114127,-0.0503237,0.140317,0.061872,
-8.36767e-09,-3.56819e-08,0.176911,-0.0632906,-0.120802,-3.88834e-08,0.110302,-0.0394611,-0.0753189,-0.00294164,-0.00129711,0.14786,0.0651981,-0.096015,-0.0423372,-0.0489038,-0.0215638,
0.0742755,0.207634,4.14973e-11,4.14987e-11,4.12039e-11,0.0493267,3.36617e-11,3.36318e-11,3.34157e-11,-0.00739817,-0.00891573,-0.00739817,-0.00891573,-0.00739817,-0.00891573,-0.00739817,-0.00891573,
-1.86494e-07,-8.23879e-07,-0.087214,-0.0862079,-0.0863961,-8.96655e-07,-0.0286318,-0.0283015,-0.0283632,0.0655115,0.0280792,-0.0215311,-0.00922902,-0.0219435,-0.00940581,-0.0220383,-0.00944643,
1.00693e-07,4.47256e-07,-0.0684805,0.122461,-0.0530658,4.86644e-07,-0.0224816,0.0402026,-0.0174211,-0.000230711,-9.87219e-05,0.0347653,0.0149013,0.0269918,0.0115694,-0.0615255,-0.0263709,
1.874e-08,1.02849e-07,-0.101027,-0.00858863,0.110553,1.11125e-07,-0.0331661,-0.00281959,0.0362936,-0.000236422,-0.000101287,0.0511842,0.0219387,-0.0555151,-0.023795,0.00456766,0.00195784;
bool check0=ref0.isApprox(tc[0],1e-5);
if(!check0){
 cout<<"tc0xV"<<endl;
   cout<<tc[0]<<endl;
    cout<<"tc0xV_ref"<<endl;
   cout<<ref0<<endl;
 }
 BOOST_CHECK_EQUAL(check0 , true);
 
 
MatrixXfd ref2=MatrixXfd::Zero(8,17);
ref2<<-5.051e-09,-1.41029e-08,-0.0319717,0.173102,-0.137513,-1.56769e-08,-0.0199341,0.107927,-0.085738,0.00148142,0.000653224,-0.0276715,-0.0122016,-0.114127,-0.0503237,0.140317,0.061872,
2.39651e-09,-8.24015e-08,-0.00317916,0.014758,-0.0125037,-1.62776e-07,-0.00254648,0.0118211,-0.0100154,-0.00286741,-0.000872611,-0.0168454,-0.00512604,-0.0746628,-0.0227195,0.0943754,0.0287178,
0.0820634,0.322473,-0.0314717,0.00581279,-0.00731716,0.321255,-0.0252087,0.00465601,-0.005861,0.0561108,0.161331,0.0654387,0.16417,0.215211,0.209744,0.296624,0.234518,
-2.01331e-09,-2.83319e-08,-0.0080701,-0.0135289,0.0215819,-3.3792e-08,-0.00646409,-0.0108366,0.0172869,-5.32447e-05,-1.62177e-05,-0.0499861,-0.0152105,0.133873,0.0407367,-0.0838337,-0.0255101,
-5.19513e-09,-1.15468e-08,-0.0169749,0.0919059,-0.0730106,1.99757e-09,0.00698531,-0.03782,0.0300444,-0.000447049,-0.000524763,0.00835033,0.00980189,0.0344397,0.0404264,-0.042343,-0.0497035,
-0.000407863,-0.00126228,0.00318885,-0.0151754,0.012692,-0.000627269,0.00841474,-0.0400447,0.0334916,0.00104815,0.000858402,0.00710388,0.00641762,0.0302775,0.0276913,-0.0376775,-0.0346923,
0.0493194,0.152633,0.0267661,-0.0114295,0.0162178,0.0758487,0.0706315,-0.030161,0.0427964,0.0157454,0.02701,0.0040689,0.0162908,-0.0216537,-0.0073228,-0.0890738,-0.0692155,
-0.0216329,-0.0669491,-0.0208961,-0.0106526,0.0177031,-0.0332693,-0.0551413,-0.0281105,0.0467155,-0.00691246,-0.0118529,-0.0241055,-0.0276363,0.0700211,0.0587731,0.000873844,-0.00470495;

bool check2=ref2.isApprox(tc[2],1e-5);
if(!check2){
 cout<<"tc2xV"<<endl;
   cout<<tc[2]<<endl;
    cout<<"tc2XV_ref"<<endl;
   cout<<ref2<<endl;
 }

BOOST_CHECK_EQUAL(check2 , true);
 

MatrixXfd ref4=MatrixXfd::Zero(8,17);
ref4<<0.0742755,0.207634,4.14974e-11,4.14988e-11,4.12039e-11,0.0493267,3.36623e-11,3.3631e-11,3.34162e-11,-0.00739817,-0.00891573,-0.00739817,-0.00891573,-0.00739817,-0.00891573,-0.00739817,-0.00891573,
-1.52634e-08,-4.66176e-08,-0.0703897,-0.0670038,-0.0679789,9.83846e-09,0.028966,0.0275727,0.0279739,0.0478093,0.0561201,-0.0150368,-0.0176506,-0.0161593,-0.0189682,-0.0166132,-0.0195011,
-5.19513e-09,-1.15468e-08,-0.0169749,0.0919059,-0.0730106,1.99757e-09,0.00698531,-0.03782,0.0300444,-0.000447049,-0.000524763,0.00835033,0.00980189,0.0344397,0.0404264,-0.042343,-0.0497035,
-6.04187e-09,-1.67166e-08,0.0939282,-0.0336032,-0.0641381,3.38373e-09,-0.0386522,0.013828,0.0263934,0.000887696,0.00104202,-0.0446193,-0.0523755,0.0289741,0.0340107,0.0147575,0.0173228,
0.0730506,0.176352,4.01847e-11,4.0171e-11,3.98884e-11,0.189719,-2.90223e-11,-2.89814e-11,-2.88023e-11,0.0924625,0.218085,0.0924625,0.218085,0.0924625,0.218085,0.0924625,0.218085,
-1.28705e-07,-3.66979e-07,-0.0478781,-0.0473258,-0.0474291,7.56414e-08,-0.0210765,-0.0208334,-0.0208789,-0.0867732,-0.143371,0.0285184,0.0471197,0.0290648,0.0480224,0.0291904,0.0482299,
6.89792e-08,1.97636e-07,-0.037594,0.0672277,-0.0291317,-4.08537e-08,-0.0165494,0.0295949,-0.0128242,0.000305601,0.000504882,-0.0460476,-0.0760823,-0.0357513,-0.0590702,0.0814931,0.134647,
8.75399e-09,3.29366e-08,-0.055461,-0.00471492,0.0606908,-7.72814e-09,-0.024415,-0.00207557,0.0267172,0.000313197,0.000517475,-0.0677951,-0.112015,0.0735318,0.121493,-0.00605002,-0.00999617;
bool check4=ref4.isApprox(tc[4],1e-5);
if(!check4){
 cout<<"tc4xV"<<endl;
   cout<<tc[4]<<endl;
    cout<<"tc4xV_ref"<<endl;
   cout<<ref4<<endl;
 }

BOOST_CHECK_EQUAL(check4 , true);
 


 
  
}
BOOST_AUTO_TEST_SUITE_END()