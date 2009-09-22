#include <UnitTest++.h>

#include "cmd_queue.h"
#include "cmd_result_parser.h"

#include "mock_command_executor.h"

// Test List
//--------------------------------
/// have Actions that execute commands
/// execute Commands
/// handle notifications from the debugger
// need a commnad executor, so we can execute commands
// every new command should have a unique id
// need a command id class
// compare CommandID
// need an increment operator in the CommandID
// map CommandID <-> output from the command
/// the output should be parsed in a ResultParser
/// test the CommandResultMap interface
/// hash function for CommandID
// CommandID operator <<
// refactor the CommnandResultMap
/// replace CommnandResultMap with some kind of CommandDispatcher
// add tests for processing the output from the debugger
/// introduce some kind of action class
// test splitting dbg output to CommandID and result string

TEST(CommnadIDToString)
{
    dbg_mi::CommandID id(1, 0);

    CHECK(id.ToString() == wxT("10000000000"));
}

TEST(CommandIDEqual)
{
    CHECK(dbg_mi::CommandID(1, 2) == dbg_mi::CommandID(1, 2));
    CHECK(dbg_mi::CommandID(1, 2) != dbg_mi::CommandID(3, 2));
}

TEST(CommandIDInc)
{
    dbg_mi::CommandID id(1, 0);
    ++id;

    CHECK(id == dbg_mi::CommandID(1, 1));
    id++;
    CHECK(id == dbg_mi::CommandID(1, 2));
}

TEST(CommandIDOutputOperator)
{
    std::stringstream s;
    dbg_mi::CommandID id(1, 0);

    s << id;

    CHECK(s.str() == id.ToString().utf8_str().data());
}

TEST(ExecuteCommand)
{
    MockCommandExecutor exec;
    dbg_mi::CommandID id = exec.Execute(wxT("-exec-run"));
    CHECK(exec.GetOutput() == id.ToString() + wxT("^running"));
}

TEST(ExecuteCommandUniqueID)
{
    MockCommandExecutor exec;

    CHECK(exec.Execute(wxT("-exec-run")) != exec.Execute(wxT("-exec-run")));
}

TEST(ExecuteGetResult)
{
    MockCommandExecutor exec;
    dbg_mi::CommandID id = exec.Execute(wxT("-exec-run"));

    dbg_mi::CommandID result_id;
    dbg_mi::ResultParser *result = exec.GetResult(result_id);

    CHECK_EQUAL(id, result_id);
    CHECK(result && !exec.HasOutput());

    delete result;
}

struct CommandResultMapInterfaceFixture
{
    CommandResultMapInterfaceFixture()
    {
        p1 = new dbg_mi::ResultParser;
        p2 = new dbg_mi::ResultParser;
        p3 = new dbg_mi::ResultParser;

        id1 = dbg_mi::CommandID(1, 1);
        id2 = dbg_mi::CommandID(1, 2);
        id3 = dbg_mi::CommandID(1, 3);

        status = map.Set(id1, p1);
        status &= map.Set(id2, p2);
        status &= map.Set(id3, p3);
    }

    dbg_mi::CommandResultMap map;
    dbg_mi::ResultParser *p1, *p2, *p3;
    dbg_mi::CommandID id1, id2, id3;

    bool status;
};

TEST_FIXTURE(CommandResultMapInterfaceFixture, Status)
{
    CHECK(status);
}

TEST_FIXTURE(CommandResultMapInterfaceFixture, Count)
{
    CHECK_EQUAL(3, map.GetCount());
}

TEST_FIXTURE(CommandResultMapInterfaceFixture, HasResults)
{
    CHECK(map.HasResult(id1) && map.HasResult(id2) && map.HasResult(id3));
}

//TEST_FIXTURE(CommandResultMapInterfaceFixture, GetResultForID)
//{
//    dbg_mi::ResultParser *p1 = map.GetResult(id1);
//}

struct CommandResultMapFixture
{
    CommandResultMapFixture()
    {
        id1 = exec.Execute(wxT("-break-insert main.cpp:400"));
        id2 = exec.Execute(wxT("-exec-run"));

        result = dbg_mi::ProcessOutput(exec, map);
    }

    MockCommandExecutor exec;
    dbg_mi::CommandID id1;
    dbg_mi::CommandID id2;
    dbg_mi::CommandResultMap map;

    bool result;
};

TEST_FIXTURE(CommandResultMapFixture, TestStatus)
{
    CHECK(result);
}

TEST_FIXTURE(CommandResultMapFixture, TestCount)
{
    CHECK_EQUAL(2, map.GetCount());
}

TEST_FIXTURE(CommandResultMapFixture, TestHasResult)
{
    CHECK(map.HasResult(id1) && map.HasResult(id2));
}


TEST(TestParseDebuggerOutputLine)
{
    wxString line = wxT("10000000005^running");

    dbg_mi::CommandID id;
    wxString result_str;

    CHECK(dbg_mi::ParseGDBOutputLine(line, id, result_str));
    CHECK_EQUAL(dbg_mi::CommandID(1, 5), id);
    CHECK(wxT("^running") == result_str);
}

void ProcessOutputTestHelper(dbg_mi::CommandExecutor &exec, dbg_mi::CommandID const &id, wxString const &command)
{
    CHECK(exec.ProcessOutput(id.ToString() + command));
    CHECK(exec.HasOutput());

    dbg_mi::CommandID result_id;
    dbg_mi::ResultParser *parser;

    parser = exec.GetResult(result_id);
    CHECK(parser && result_id != dbg_mi::CommandID() && result_id == id);
}

TEST(DebuggerOutputParallel)
{
    MockCommandExecutor exec(false);
    dbg_mi::CommandID id1 = exec.Execute(wxT("-break-insert main.cpp:400"));
    dbg_mi::CommandID id2 = exec.Execute(wxT("-exec-run"));

    ProcessOutputTestHelper(exec, id1, wxT("^running"));
    ProcessOutputTestHelper(exec,
                            id2,
                            wxT("^done,bkpt={number=\"1\",addr=\"0x0001072c\",file=\"main.cpp\",")
                            wxT("fullname=\"/home/foo/main.cpp\",line=\"4\",times=\"0\"}"));
}

TEST(DebuggerOutputSequential)
{
    MockCommandExecutor exec(false);
    dbg_mi::CommandID id1 = exec.Execute(wxT("-break-insert main.cpp:400"));
    ProcessOutputTestHelper(exec, id1, wxT("^running"));

    dbg_mi::CommandID id2 = exec.Execute(wxT("-exec-run"));
    ProcessOutputTestHelper(exec,
                            id2,
                            wxT("^done,bkpt={number=\"1\",addr=\"0x0001072c\",file=\"main.cpp\",")
                            wxT("fullname=\"/home/foo/main.cpp\",line=\"4\",times=\"0\"}"));
}
