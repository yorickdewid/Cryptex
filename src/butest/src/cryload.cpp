// Copyright (c) 2017 Quenza Inc. All rights reserved.
//
// This file is part of the Cryptox project.
//
// Use of this source code is governed by a private license
// that can be found in the LICENSE file. Content can not be 
// copied and/or distributed without the express of the author.

#include <Cry/Cry.h>
#include <Cry/Loader.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(CryLoader)

BOOST_AUTO_TEST_CASE(ModLoadBasic)
{
	Cry::Module::Module mod = Cry::Module::Load("test\\mod_test1.dll");
	mod.Load();
	BOOST_REQUIRE_EQUAL("mod_example1", mod.Name());
	BOOST_REQUIRE_EQUAL("Blub Corp.", mod.Author());
	BOOST_REQUIRE_EQUAL("Copyright (c) 2018", mod.Copyright());
	mod.Unload();
}

BOOST_AUTO_TEST_SUITE_END()
