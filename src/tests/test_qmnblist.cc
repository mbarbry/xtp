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

#define BOOST_TEST_MODULE qmnblist_test
#include <boost/test/unit_test.hpp>
#include <votca/xtp/qmnblist.h>

using namespace votca::xtp;

BOOST_AUTO_TEST_SUITE(qmnblist_test)

BOOST_AUTO_TEST_CASE(constructors_test) { 
  QMNBList qmnb; 
  QMNBList::SuperExchangeType sext("Seg1 Seg2 Seg3");
}


BOOST_AUTO_TEST_CASE(superexchange_test) { 
  QMNBList::SuperExchangeType sext("seg2 seg3 seg4");

  BOOST_CHECK_EQUAL(sext.isOfBridge("seg1"),false);
  BOOST_CHECK_EQUAL(sext.isOfDonorAcceptor("seg1"),false);
  BOOST_CHECK_EQUAL(sext.isOfBridge("seg3"),true);
  BOOST_CHECK_EQUAL(sext.isOfDonorAcceptor("seg2"),true);
  BOOST_CHECK_EQUAL(sext.isOfDonorAcceptor("seg4"),true);
}

BOOST_AUTO_TEST_CASE(cutoff_test){
  QMNBList qmnb;
  qmnb.setCutoff(23.4);
  BOOST_CHECK_EQUAL(static_cast<int>(qmnb.getCutoff()*10),234);
}

BOOST_AUTO_TEST_SUITE_END()
