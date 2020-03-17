/*
 Tests of the Access ODBC connection.

 Copyright (C) 2012 AMPL Optimization Inc

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted,
 provided that the above copyright notice appear in all copies and that
 both that the copyright notice and this permission notice and warranty
 disclaimer appear in supporting documentation.

 The author and AMPL Optimization Inc disclaim all warranties with
 regard to this software, including all implied warranties of
 merchantability and fitness.  In no event shall the author be liable
 for any special, indirect or consequential damages or any damages
 whatsoever resulting from loss of use, data or profits, whether in an
 action of contract, negligence or other tortious action, arising out
 of or in connection with the use or performance of this software.

 Author: Victor Zverovich
 */

#include <cctype>
#include <climits>

#include "gtest/gtest.h"
#include "format.h"
#include "function.h"
#include "odbc.h"
#undef VOID

using fun::Handler;
using fun::Table;

#ifndef MP_TEST_DATA_DIR
# define MP_TEST_DATA_DIR "../data"
#endif

namespace {

class AccessTest : public ::testing::Test {
 protected:
  static fun::Library lib_;
  const Handler *handler_;
  std::vector<std::string> strings_;

  static void SetUpTestCase() {
    lib_.Load();
  }

  void SetUp() {
    handler_ = lib_.GetHandler("odbc");
    strings_.push_back("odbc");
    strings_.push_back(MP_TEST_DATA_DIR "/test.accdb");
  }
};

fun::Library AccessTest::lib_(AMPLTABL_DLL_NAME);

TEST_F(AccessTest, Read) {
  Table t("Test", 1, 0, strings_);
  t = "N";
  handler_->Read(&t);
  EXPECT_EQ(1u, t.num_rows());
  EXPECT_EQ(42, t(0, 0).number());
}

TEST_F(AccessTest, ReadNullField) {
  Table t("TableWithNullField", 1, 0, strings_);
  t = "NullField";
  handler_->Read(&t);
  EXPECT_EQ(1u, t.num_rows());
  EXPECT_EQ(fun::VOID, t(0, 0).type());
}

TEST_F(AccessTest, Write) {
  {
    Table t("T", 1, 0, strings_);
    t = "N", 111, 222;
    handler_->Write(t);
  }
  {
    Table t("T", 1, 0, strings_);
    t = "N";
    handler_->Read(&t);
    EXPECT_EQ(2u, t.num_rows());
    EXPECT_EQ(111, t(0, 0).number());
    EXPECT_EQ(222, t(1, 0).number());
  }
}
}
