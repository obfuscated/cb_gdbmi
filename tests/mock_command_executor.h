#ifndef _TESTS_MOCK_COMMAND_EXECUTOR_H_
#define _TESTS_MOCK_COMMAND_EXECUTOR_H_

#include <deque>

#include "cmd_queue.h"

class MockCommandExecutor : public dbg_mi::CommandExecutor
{
    struct Result
    {
        dbg_mi::CommandID id;
        wxString output;
    };
public:
    MockCommandExecutor(bool auto_process_output = true) :
        m_last(0),
        m_auto_process_output(auto_process_output)
    {
    }

    virtual wxString GetOutput() { return m_result; }
    virtual bool HasOutput() const { return m_results.size() > 0; }

    virtual bool ProcessOutput(wxString const &output)
    {
        dbg_mi::CommandID id;
        Result r;

        if(!dbg_mi::ParseGDBOutputLine(output, r.id, r.output))
            return false;

        m_results.push_back(r);
        return true;
    }

    virtual dbg_mi::ResultParser* GetResult(dbg_mi::CommandID &id)
    {
        assert(!m_results.empty());
        Result const &r = m_results.front();

        id = r.id;
        dbg_mi::ResultParser *parser = new dbg_mi::ResultParser;
        if(!parser->Parse(r.output.substr(1, r.output.length() - 1), dbg_mi::ResultParser::ParseType(r.output)))
        {
            delete parser;
            parser = NULL;
        }
        m_results.pop_front();
        return parser;
    }
protected:
    virtual dbg_mi::CommandID DoExecute(wxString const &cmd)
    {
        Result r;
        r.id = dbg_mi::CommandID(1, m_last++);
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
                m_result = r.id.ToString() + r.output;
                m_results.push_back(r);
            }
            return r.id;
        }
        else
            return dbg_mi::CommandID();
    }

private:
    wxString m_result;
    int32_t m_last;

    typedef std::deque<Result> Results;
    Results m_results;
    bool m_auto_process_output;
};


#endif // _TESTS_MOCK_COMMAND_EXECUTOR_H_

