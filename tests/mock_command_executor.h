#ifndef _TESTS_MOCK_COMMAND_EXECUTOR_H_
#define _TESTS_MOCK_COMMAND_EXECUTOR_H_

#include <deque>

#include "cmd_queue.h"

class MockCommandExecutor : public dbg_mi::CommandExecutor
{
public:
    MockCommandExecutor(bool auto_process_output = true) :
        m_auto_process_output(auto_process_output)
    {
    }

//    virtual void ExecuteSimple(dbg_mi::CommandID const &id, wxString const &cmd)
//    {
//        DoExecute(id, cmd);
//    }

    virtual wxString GetOutput() { return m_result; }

protected:
//    virtual dbg_mi::CommandID DoExecute(wxString const &cmd)
//    {
//        dbg_mi::CommandID id(1, m_last++);
//        if(DoExecute(id, cmd))
//            return id;
//        else
//            return dbg_mi::CommandID();
//    }

    bool DoExecute(dbg_mi::CommandID const &id, wxString const &cmd)
    {
        Result r;
        r.id = id;
        if(cmd == wxT("-exec-run"))
        {
            r.output = wxT("^running");
        }
        else if(cmd.StartsWith(wxT("-break-insert")))
        {
            r.output = wxT("^done,bkpt={number=\"1\",addr=\"0x0001072c\",file=\"main.cpp\",")
                       wxT("fullname=\"/home/foo/main.cpp\",line=\"4\",times=\"0\"}");

        }

        if(!r.output.empty())
        {
            if(m_auto_process_output)
            {
                ProcessOutput(r.id.ToString() + r.output);
                m_result = r.id.ToString() + r.output;
            }
            return true;
        }
        else
            return false;
    }
private:
    wxString m_result;

    bool m_auto_process_output;
};


#endif // _TESTS_MOCK_COMMAND_EXECUTOR_H_

