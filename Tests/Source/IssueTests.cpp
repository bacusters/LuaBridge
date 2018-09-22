// https://github.com/vinniefalco/LuaBridge
//
// Copyright 2018, Dmitry Tarakanov
// SPDX-License-Identifier: MIT

#include "TestBase.h"

struct IssueTests : TestBase
{
};

struct Track
{
  std::string name;
};

#include <LuaBridge/Vector.h>

int initSongs(lua_State* L)
{
   std::vector <Track*> tracks;
   tracks.push_back (new Track {"Song1"});
   tracks.push_back (new Track {"Song2"});
   luabridge::push (L, tracks);
   return 1;
}

TEST_F (IssueTests, Issue76)
{
  luabridge::getGlobalNamespace (L)
    .beginClass <Track> ("Track")
    .addData ("name", &Track::name)
    .endClass ()
    .addCFunction ("init_songs", &initSongs);

    runLua (
      "local songs = init_songs () "
      "for index, song in ipairs (songs) do "
      "    print (song.name) "
      "end");
}

struct AbstractClass
{
  virtual int sum (int a, int b) = 0;
};

struct ConcreteClass : AbstractClass
{
  int sum (int a, int b) override
  {
    return a + b;
  }

  static AbstractClass& get()
  {
    static ConcreteClass instance;
    return instance;
  }
};

TEST_F (IssueTests, Issue87)
{
  luabridge::getGlobalNamespace (L)
    .beginClass <AbstractClass> ("Class")
    .addFunction ("sum", &AbstractClass::sum)
    .endClass ()
    .addFunction ("getAbstractClass", &ConcreteClass::get);

  runLua ("result = getAbstractClass():sum (1, 2)");
  ASSERT_TRUE (result ().isNumber ());
  ASSERT_EQ (3, result ().cast <int> ());
}

TEST_F (IssueTests, Issue121)
{
  runLua (R"(
    first = {
      second = {
        actual = "data"
      }
    }
  )");
  auto first = luabridge::getGlobal (L, "first");
  ASSERT_TRUE (first.isTable ());
  ASSERT_EQ (0, first.length ());
  ASSERT_TRUE (first ["second"].isTable ());
  ASSERT_EQ (0, first ["second"].length ());
}

void pushArgs (lua_State*)
{
}

template <class Arg, class... Args>
void pushArgs (lua_State* L, Arg arg, Args... args)
{
  luabridge::Stack <Arg>::push (L, arg);
  pushArgs (L, args...);
}

template <class... Args>
std::vector <luabridge::LuaRef> callFunction (const luabridge::LuaRef& function, Args... args)
{
  assert (function.isFunction ());

  lua_State* L = function.state ();
  int originalTop = lua_gettop (L);
  function.push (L);
  pushArgs (L, args...);

  luabridge::LuaException::pcall (L, sizeof... (args), LUA_MULTRET);

  std::vector <luabridge::LuaRef> results;
  int top = lua_gettop (L);
  results.reserve (top - originalTop);
  for (int i = originalTop + 1; i <= top; ++i)
  {
    results.push_back (luabridge::LuaRef::fromStack (L, i));
  }
  return results;
}

TEST_F (IssueTests, Issue160)
{
  runLua (
    "function isConnected (arg1, arg2) "
    " return 1, 'srvname', 'ip:10.0.0.1', arg1, arg2 "
    "end");

  luabridge::LuaRef f_isConnected = luabridge::getGlobal (L, "isConnected");

  auto v = callFunction (f_isConnected, 2, "abc");
  ASSERT_EQ (5, v.size ());
  ASSERT_EQ (1, v [0].cast <int> ());
  ASSERT_EQ ("srvname", v [1].cast <std::string> ());
  ASSERT_EQ ("ip:10.0.0.1", v [2].cast <std::string> ());
  ASSERT_EQ (2, v [3].cast <int> ());
  ASSERT_EQ ("abc", v [4].cast <std::string> ());
}
