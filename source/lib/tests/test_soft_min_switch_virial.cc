// SPDX-License-Identifier: LGPL-3.0-or-later
#include <gtest/gtest.h>

#include <iostream>

#include "env_mat.h"
#include "fmt_nlist.h"
#include "neighbor_list.h"
#include "soft_min_switch.h"
#include "soft_min_switch_virial.h"

class TestSoftMinSwitchVirial : public ::testing::Test {
 protected:
  std::vector<double> posi = {12.83, 2.56, 2.18, 12.09, 2.87, 2.74,
                              00.25, 3.32, 1.68, 3.36,  3.00, 1.81,
                              3.51,  2.51, 2.60, 4.27,  3.22, 1.56};
  std::vector<int> atype = {0, 1, 1, 0, 1, 1};
  std::vector<double> posi_cpy;
  std::vector<int> atype_cpy;
  int ntypes = 2;
  int nloc, nall, nnei, ndescrpt;
  double rc = 6;
  double rc_smth = 0.8;
  double alpha = .5;
  double rmin = 0.8;
  double rmax = 1.5;
  SimulationRegion<double> region;
  std::vector<int> mapping, ncell, ngcell;
  std::vector<int> sec_a = {0, 5, 10};
  std::vector<int> sec_r = {0, 0, 0};
  std::vector<int> nat_stt, ext_stt, ext_end;
  std::vector<std::vector<int>> nlist_a_cpy, nlist_r_cpy;
  std::vector<double> sw_value, sw_deriv, du;
  std::vector<double> rij;
  std::vector<int> nlist;
  std::vector<int> fmt_nlist_a;
  std::vector<double> expected_virial = {
      3.06079,  0.53537,  -2.41160, 0.53537, 2.78437,
      -1.61110, -2.41160, -1.61110, 3.49276,
  };
  std::vector<double> expected_atom_virial = {
      1.35376,  -0.56712, -1.02447, -0.56712, 0.23758,  0.42917,  -1.02447,
      0.42917,  0.77527,  0.54989,  -0.23036, -0.41613, -0.23036, 0.09650,
      0.17433,  -0.41613, 0.17433,  0.31491,  -0.19229, 0.02835,  -0.02453,
      0.02835,  -0.00576, 0.00594,  -0.02453, 0.00594,  -0.00660, 0.87609,
      0.15772,  -0.10844, 0.15772,  0.43891,  -0.68116, -0.10844, -0.68116,
      1.06620,  -0.07197, 0.02107,  -0.01394, 0.02107,  0.17539,  -0.28148,
      -0.01394, -0.28148, 0.45073,  0.41749,  0.14761,  -0.17569, 0.14761,
      0.06082,  -0.08056, -0.17569, -0.08056, 0.10804,  0.25593,  0.69163,
      -0.46058, 0.69163,  1.26326,  -0.83076, -0.46058, -0.83076, 0.54550,
      -0.01118, 0.01474,  -0.02646, 0.01474,  0.00500,  -0.01216, -0.02646,
      -0.01216, 0.02792,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.14854,  0.28167,  -0.17934, 0.28167,  0.51470,  -0.33630,
      -0.17934, -0.33630, 0.21579,  -0.12311, -0.01078, 0.01838,  -0.01078,
      -0.00120, 0.00130,  0.01838,  0.00130,  -0.00312, -0.11042, 0.00462,
      -0.00589, 0.00462,  -0.00032, -0.00002, -0.00589, -0.00002, -0.00089,
      -0.03191, -0.00379, 0.00549,  -0.00379, -0.00050, 0.00060,  0.00549,
      0.00060,  -0.00100, 0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
      0.00000,  0.00000,  0.00000,  0.00000,  0.00000,
  };

  void SetUp() override {
    double box[] = {13., 0., 0., 0., 13., 0., 0., 0., 13.};
    region.reinitBox(box);
    copy_coord(posi_cpy, atype_cpy, mapping, ncell, ngcell, posi, atype, rc,
               region);
    nloc = posi.size() / 3;
    nall = posi_cpy.size() / 3;
    nnei = sec_a.back();
    ndescrpt = nnei * 4;
    nat_stt.resize(3);
    ext_stt.resize(3);
    ext_end.resize(3);
    for (int dd = 0; dd < 3; ++dd) {
      ext_stt[dd] = -ngcell[dd];
      ext_end[dd] = ncell[dd] + ngcell[dd];
    }
    build_nlist(nlist_a_cpy, nlist_r_cpy, posi_cpy, nloc, rc, rc, nat_stt,
                ncell, ext_stt, ext_end, region, ncell);
    nlist.resize(nloc * nnei);
    rij.resize(nloc * nnei * 3);
    for (int ii = 0; ii < nloc; ++ii) {
      // format nlist and record
      format_nlist_i_cpu<double>(fmt_nlist_a, posi_cpy, atype_cpy, ii,
                                 nlist_a_cpy[ii], rc, sec_a);
      for (int jj = 0; jj < nnei; ++jj) {
        nlist[ii * nnei + jj] = fmt_nlist_a[jj];
      }
      std::vector<double> t_env, t_env_deriv, t_rij;
      // compute env_mat and its deriv, record
      deepmd::env_mat_a_cpu<double>(t_env, t_env_deriv, t_rij, posi_cpy,
                                    atype_cpy, ii, fmt_nlist_a, sec_a, rc_smth,
                                    rc);
      for (int jj = 0; jj < nnei * 3; ++jj) {
        rij[ii * nnei * 3 + jj] = t_rij[jj];
      }
    }
    sw_value.resize(nloc);
    sw_deriv.resize(nloc * nnei * 3);
    deepmd::soft_min_switch_cpu<double>(&sw_value[0], &sw_deriv[0], &rij[0],
                                        &nlist[0], nloc, nnei, alpha, rmin,
                                        rmax);
    du.resize(nloc);
    for (int ii = 0; ii < nloc; ++ii) {
      du[ii] = 1.0 - ii * 0.1;
    }
  }
  void TearDown() override {}
};

TEST_F(TestSoftMinSwitchVirial, cpu) {
  std::vector<double> virial(9);
  std::vector<double> atom_virial(nall * 9);
  deepmd::soft_min_switch_virial_cpu(&virial[0], &atom_virial[0], &du[0],
                                     &sw_deriv[0], &rij[0], &nlist[0], nloc,
                                     nall, nnei);
  EXPECT_EQ(virial.size(), expected_virial.size());
  for (int jj = 0; jj < virial.size(); ++jj) {
    EXPECT_LT(fabs(virial[jj] - expected_virial[jj]), 1e-5);
  }
  EXPECT_EQ(atom_virial.size(), expected_atom_virial.size());
  for (int jj = 0; jj < atom_virial.size(); ++jj) {
    EXPECT_LT(fabs(atom_virial[jj] - expected_atom_virial[jj]), 1e-5);
  }
  // for (int ii = 0; ii < 9; ++ii){
  //   printf("%8.5f, ", virial[ii]);
  // }
  // for (int ii = 0; ii < 9 * nall; ++ii){
  //   printf("%8.5f, ", atom_virial[ii]);
  // }
  // printf("\n");
}
