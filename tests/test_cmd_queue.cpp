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
/// replace CommnandResultMap with some kind of CommandResultDispatcher
// add tests for processing the output from the debugger
// introduce some kind of action class
// test splitting dbg output to CommandID and result string
// execute commands inside action derivates or some user code
// add actions to CommandExecutor
/// notification should go to some global place
// extract actions from the CommandExecutor to ActionsMap
// remove actions from the ActionsMap when they have finished
// processes commands for finished actions
/// add some metrics for the count of actions in the ActionsMap
// add ids to actions
// action ids should be set automatically
/// associate CommandIDs to the commands executed for every action

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

bool ProcessOutputTestHelper(dbg_mi::CommandExecutor &exec, dbg_mi::CommandID const &id, wxString const &command)
{
    if(!exec.ProcessOutput(id.ToString() + command))
        return false;
    if(!exec.HasOutput())
        return false;

    dbg_mi::CommandID result_id;
    dbg_mi::ResultParser *parser;

    parser = exec.GetResult(result_id);
    return parser && result_id != dbg_mi::CommandID() && result_id == id;
}

bool ProcessOutputTestHelperSimple(dbg_mi::CommandExecutor &exec)
{
    if(!exec.HasOutput())
        return false;

    dbg_mi::CommandID result_id;
    dbg_mi::ResultParser *parser = exec.GetResult(result_id);
    return parser && result_id != dbg_mi::CommandID();
}

bool ProcessOutputTestResult(dbg_mi::CommandExecutor &exec, dbg_mi::CommandID const &id, wxString const &result)
{
    if(!exec.HasOutput())
        return false;

    dbg_mi::CommandID result_id;
    dbg_mi::ResultParser *parser;

    parser = exec.GetResult(result_id);
    return parser && result_id != dbg_mi::CommandID() && result_id == id;
}

TEST(DebuggerOutputParallel)
{
    MockCommandExecutor exec(false);
    dbg_mi::CommandID id1 = exec.Execute(wxT("-break-insert main.cpp:400"));
    dbg_mi::CommandID id2 = exec.Execute(wxT("-exec-run"));

    CHECK(ProcessOutputTestHelper(exec, id1, wxT("^running")));
    CHECK(ProcessOutputTestHelper(exec,
                                  id2,
                                  wxT("^done,bkpt={number=\"1\",addr=\"0x0001072c\",file=\"main.cpp\",")
                                  wxT("fullname=\"/home/foo/main.cpp\",line=\"4\",times=\"0\"}"))
          );
}

TEST(DebuggerOutputSequential)
{
    MockCommandExecutor exec(false);
    dbg_mi::CommandID id1 = exec.Execute(wxT("-break-insert main.cpp:400"));
    CHECK(ProcessOutputTestHelper(exec, id1, wxT("^running")));

    dbg_mi::CommandID id2 = exec.Execute(wxT("-exec-run"));
    CHECK(ProcessOutputTestHelper(exec,
                                  id2,
                                  wxT("^done,bkpt={number=\"1\",addr=\"0x0001072c\",file=\"main.cpp\",")
                                  wxT("fullname=\"/home/foo/main.cpp\",line=\"4\",times=\"0\"}"))
          );
}

struct TestAction : public dbg_mi::Action
{
    TestAction(bool *destroyed = NULL) :
        on_start_called(false),
        m_destroyed(destroyed)
    {
    }
    ~TestAction()
    {
        if(m_destroyed)
            *m_destroyed = true;
    }

    virtual void OnStart()
    {
        on_start_called = true;
    }

    virtual void OnCommandOutput(dbg_mi::CommandID const &id, wxString const &output_)
    {
        command_id = id;
        output = output_;
        Finish();
    }

    dbg_mi::CommandID command_id;
    wxString output;
    bool on_start_called;
private:
    bool *m_destroyed;
};

TEST(ActionInterfaceCtor)
{
    TestAction action;
    CHECK(!action.Started() && !action.Finished());
}

TEST(ActionInterfaceStartingFinishing)
{
    TestAction action;
    action.Start();
    CHECK(action.Started() && !action.Finished());

    action.Finish();
    CHECK(action.Started() && action.Finished());
    CHECK_EQUAL(-1, action.GetID());
    CHECK(!action.HasPendingCommands());
}

TEST(ActionInterfaceOnStart)
{
    TestAction action;
    action.Start();
    CHECK(action.Started() && action.on_start_called);
}

TEST(ActionInterfaceOnCmdOutput)
{
    TestAction action;
    action.Start();

    dbg_mi::CommandID id(100, 1);
    wxString output = wxT("^running");
    action.OnCommandOutput(id, output);

    CHECK(action.command_id == id && action.output == output);
    CHECK(action.Finished());
}

TEST(ActionInterfaceExecCommands)
{
    TestAction action;
    action.Execute(wxT("-break-insert main.cpp"));
    action.Execute(wxT("-exec-run"));

    CHECK_EQUAL(2, action.GetPendingCommandsCount());
    dbg_mi::CommandID id;
    CHECK(action.PopPendingCommand(id) == wxT("-break-insert main.cpp"));
    CHECK(action.PopPendingCommand(id) == wxT("-exec-run"));
}

TEST(ActionInterfaceIDs)
{
    TestAction action;

    action.SetID(10);
    CHECK_EQUAL(10, action.GetID());
}

TEST(ActionInterfaceExecuteCommandID)
{
    TestAction a;
    a.SetID(10);

    int id1 = a.Execute(wxT("-exec-run"));
    int id2 = a.Execute(wxT("-exec-next"));

    CHECK_EQUAL(0, id1);
    CHECK_EQUAL(1, id2);
}

TEST(ActionMapAutomaticActionID)
{
    TestAction *a1 = new TestAction;
    TestAction *a2 = new TestAction;
    dbg_mi::ActionsMap actions;

    actions.Add(a1);
    actions.Add(a2);

    CHECK_EQUAL(1, a1->GetID());
    CHECK_EQUAL(2, a2->GetID());
}

struct ActionsMapFixture
{
    ActionsMapFixture()
    {
        action = new TestAction(&action_destroyed);
        actions_map.Add(action);
        action->SetID(142);
    }

    dbg_mi::ActionsMap actions_map;
    TestAction *action;
    MockCommandExecutor exec;
    bool action_destroyed;
};

TEST_FIXTURE(ActionsMapFixture, Empty)
{
    CHECK(!actions_map.Empty());
}

TEST_FIXTURE(ActionsMapFixture, ActionStarted)
{
    actions_map.Run(exec);
    CHECK(action->Started());
}

TEST_FIXTURE(ActionsMapFixture, ActionDestroyed)
{
    action->Finish();
    actions_map.Run(exec);
    CHECK(action_destroyed);
}

TEST_FIXTURE(ActionsMapFixture, ActionExecute)
{
    action->Execute(wxT("-break-insert main.cpp:10"));
    action->Execute(wxT("-break-insert main.cpp:20"));
    actions_map.Run(exec);
    ProcessOutputTestHelperSimple(exec);
    ProcessOutputTestHelperSimple(exec);
}

TEST_FIXTURE(ActionsMapFixture, ActionExecuteAndFinish)
{
    action->Execute(wxT("-break-insert main.cpp:10"));
    action->Execute(wxT("-break-insert main.cpp:20"));
    action->Finish();
    actions_map.Run(exec);
    CHECK(ProcessOutputTestHelperSimple(exec));
    CHECK(ProcessOutputTestHelperSimple(exec));
}

TEST_FIXTURE(ActionsMapFixture, ActionExecuteCheckIDs)
{
    action->Execute(wxT("-exec-run"));
    action->Execute(wxT("-exec-run"));
    actions_map.Run(exec);
    CHECK(ProcessOutputTestResult(exec, dbg_mi::CommandID(action->GetID(), 0), wxT("^running")));
    CHECK(ProcessOutputTestResult(exec, dbg_mi::CommandID(action->GetID(), 1), wxT("^running")));
}

//TEST(Dispatcher)
//{
//    MockCommandExecutor exec;
//
//    TestAction action1;
//    TestAction action2;
//
//    action1.Execute(wxT("-break-insert main.cpp:10"));
//    action1.Execute(wxT("-break-insert main.cpp:20"));
//
//    action2.Execute(wxT("-exec-run"));
//
//    exec.
//}
