#include "actions.h"

#include <backtracedlg.h>
#include <cbplugin.h>
#include <logmanager.h>
#include <threadsdlg.h>
#include <watchesdlg.h>

#include "cmd_result_parser.h"
#include "frame.h"

namespace dbg_mi
{

void BreakpointAddAction::OnStart()
{
    wxString cmd(wxT("-break-insert "));
    cbBreakpoint &bp = m_breakpoint->Get();

    if(bp.UseCondition())
        cmd += wxT("-c ") + bp.GetCondition() + wxT(" ");
    if(bp.UseIgnoreCount())
        cmd += wxT("-i ") + wxString::Format(wxT("%d "), bp.GetIgnoreCount());

    cmd += wxString::Format(wxT("%s:%d"), bp.GetFilename().c_str(), bp.GetLine());
    m_initial_cmd = Execute(cmd);
    m_logger.Debug(wxT("BreakpointAddAction::m_initial_cmd = ") + m_initial_cmd.ToString());
}

void BreakpointAddAction::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    m_logger.Debug(wxT("BreakpointAddAction::OnCommandResult: ") + id.ToString());

    if(m_initial_cmd == id)
    {
        const ResultValue &value = result.GetResultValue();
        const ResultValue *number = value.GetTupleValue(wxT("bkpt.number"));
        if(number)
        {
            const wxString &number_value = number->GetSimpleValue();
            long n;
            if(number_value.ToLong(&n, 10))
            {
                m_logger.Debug(wxString::Format(wxT("BreakpointAddAction::breakpoint index is %d"), n));
                m_breakpoint->SetIndex(n);

                if(!m_breakpoint->Get().IsEnabled())
                    m_disable_cmd = Execute(wxString::Format(wxT("-break-disable %d"), n));
                else
                {
                    m_logger.Debug(wxT("BreakpointAddAction::Finishing1"));
                    Finish();
                }
            }
            else
                m_logger.Debug(wxT("BreakpointAddAction::error getting the index :( "));
        }
        else
        {
            m_logger.Debug(wxT("BreakpointAddAction::error getting number value:( "));
            m_logger.Debug(value.MakeDebugString());
        }
    }
    else if(m_disable_cmd == id)
    {
        m_logger.Debug(wxT("BreakpointAddAction::Finishing2"));
        Finish();
    }
}

GenerateBacktrace::GenerateBacktrace(BacktraceContainer &backtrace, Logger &logger) :
    m_backtrace(backtrace),
    m_logger(logger),
    m_parsed_backtrace(false),
    m_parsed_args(false)
{
}
void GenerateBacktrace::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    if(id == m_backtrace_id)
    {
        ResultValue const *stack = result.GetResultValue().GetTupleValue(wxT("stack"));
        if(!stack)
            m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput: no stack tuple in the output"));
        else
        {
            int size = stack->GetTupleSize();
            m_logger.Debug(wxString::Format(wxT("GenerateBacktrace::OnCommandOutput: tuple size %d %s"),
                                            size, stack->MakeDebugString().c_str()));

            m_backtrace.clear();

            for(int ii = 0; ii < size; ++ii)
            {
                ResultValue const *frame_value = stack->GetTupleValueByIndex(ii);
                assert(frame_value);
                Frame frame;
                if(frame.ParseFrame(*frame_value))
                {
                    cbStackFrame s;
                    if(frame.HasValidSource())
                        s.SetFile(frame.GetFilename(), wxString::Format(wxT("%d"), frame.GetLine()));
                    else
                        s.SetFile(frame.GetFrom(), wxEmptyString);
                    s.SetSymbol(frame.GetFunction());
                    s.SetNumber(ii);
                    s.SetAddress(frame.GetAddress());
                    s.MakeValid(frame.HasValidSource());
                    m_backtrace.push_back(s);
                }
                else
                    m_logger.Debug(wxT("can't parse frame: ") + frame_value->MakeDebugString());
            }
        }
        m_parsed_backtrace = true;
    }
    else if(id == m_args_id)
    {
        m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput arguments"));
        dbg_mi::FrameArguments arguments;

        if(!arguments.Attach(result.GetResultValue()))
        {
            m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput: can't attach to output of command: ")
                           + id.ToString());
        }
        else if(arguments.GetCount() != static_cast<int>(m_backtrace.size()))
        {
            m_logger.Debug(wxT("GenerateBacktrace::OnCommandOutput: stack arg count differ from the number of frames"));
        }
        else
        {
            int size = arguments.GetCount();
            for(int ii = 0; ii < size; ++ii)
            {
                wxString args;
                if(arguments.GetFrame(ii, args))
                    m_backtrace[ii].SetSymbol(m_backtrace[ii].GetSymbol() + wxT("(") + args + wxT(")"));
                else
                {
                    m_logger.Debug(wxString::Format(wxT("GenerateBacktrace::OnCommandOutput: ")
                                                    wxT("can't get args for frame %d"),
                                                    ii));
                }
            }
        }
        m_parsed_args = true;
    }

    if(m_parsed_backtrace && m_parsed_args)
    {
        Manager::Get()->GetDebuggerManager()->GetBacktraceDialog()->Reload();
        Finish();
    }
}
void GenerateBacktrace::OnStart()
{
    m_backtrace_id = Execute(wxT("-stack-list-frames 0 30"));
    m_args_id = Execute(wxT("-stack-list-arguments 1 0 30"));
}

GenerateThreadsList::GenerateThreadsList(ThreadsContainer &threads, int current_thread_id, Logger &logger) :
    m_threads(threads),
    m_logger(logger),
    m_current_thread_id(current_thread_id)
{
}

void GenerateThreadsList::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    Finish();
    dbg_mi::ResultValue const *threads = result.GetResultValue().GetTupleValue(wxT("thread-ids"));
    if(!threads)
    {
        m_logger.Debug(wxT("GenerateThreadsList::OnCommandOutput - wrong command output"));
        return;
    }
    m_logger.Debug(wxString::Format(wxT("GenerateThreadsList::OnCommandOutput - parsed %s"),
                                    threads->MakeDebugString().c_str()));

    int count = threads->GetTupleSize();
    m_threads.clear();
    for(int ii = 0; ii < count; ++ii)
    {
        dbg_mi::ResultValue const &thread_value = *threads->GetTupleValueByIndex(ii);

        int number;

        if(dbg_mi::ToInt(thread_value, number))
        {
            m_logger.Debug(wxString::Format(wxT("GenerateThreadsList::OnCommandOutput - parsed %s %d"),
                                            thread_value.MakeDebugString().c_str(), number));
            m_threads.push_back(cbThread(m_current_thread_id == number, number, wxEmptyString));
        }
        else
        {
            m_logger.Debug(wxString::Format(wxT("GenerateThreadsList::OnCommandOutput - can't parse %s"),
                                            thread_value.MakeDebugString().c_str()));
        }
    }

    Manager::Get()->GetDebuggerManager()->GetThreadsDialog()->Reload();
}

void GenerateThreadsList::OnStart()
{
    Execute(wxT("-thread-list-ids"));
}

WatchCreateAction::WatchCreateAction(Watch::Pointer const &watch, Logger &logger) :
    m_watch(watch),
    m_step(StepCreate),
    m_logger(logger),
    m_sub_commands_left(0)
{
}

WatchCreateAction::~WatchCreateAction()
{
    Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->UpdateWatches();
}

int WatchCreateAction::ParseSingle(Watch &watch, ResultValue const &value, bool &has_type)
{
    int children = -1;
    wxString s;

    if(Lookup(value, wxT("type"), s))
    {
        has_type = true;
        watch.SetType(s);
    }
    else
        has_type = false;

    if(Lookup(value, wxT("name"), s))
        watch.SetID(s);

    if(Lookup(value, wxT("numchild"), children))
    {
//        if(children == 0)
        {
            if(Lookup(value, wxT("value"), s))
                watch.SetValue(s);
        }
        return children;
    }

    return -1;
}

void WatchCreateAction::ExecuteListCommand(Watch &watch, Watch *parent)
{
    CommandID const &id = Execute(wxString::Format(wxT("-var-list-children 2 \"%s\""), watch.GetID().c_str()));

    m_parent_map[id] = parent ? parent : &watch;
    ++m_sub_commands_left;
}

void WatchCreateAction::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    --m_sub_commands_left;
    m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - processing command ") + id.ToString());
    bool error = false;
    if(result.GetResultClass() == ResultParser::ClassDone)
    {
        ResultValue const &value = result.GetResultValue();
        switch(m_step)
        {
        case StepCreate:
            {
                bool has_type;
                int children = ParseSingle(*m_watch, value, has_type);
                if(children > 0)
                {
                    ExecuteListCommand(*m_watch, NULL);
                    m_step = StepListChildren;
                }
                else
                    Finish();
            }
            break;
        case StepListChildren:
            {
                m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - steplistchildren for id: ")
                               + id.ToString() + wxT(" -> ") + result.MakeDebugString());
                bool finish = true;
                ListCommandParentMap::iterator it = m_parent_map.find(id);
                Watch *parent_watch;
                if(it == m_parent_map.end() || !it->second)
                {
                    m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - no parent for id: ") + id.ToString());
                    error = true;
                    break;
                }

                parent_watch = it->second;

                ResultValue const *children = value.GetTupleValue(wxT("children"));
                if(children)
                {
                    int count = children->GetTupleSize();

                    m_logger.Debug(wxString::Format(wxT("WatchCreateAction::OnCommandOutput - children %d"), count));
                    for(int ii = 0; ii < count; ++ii)
                    {
                        ResultValue const *child_value;
                        child_value = children->GetTupleValueByIndex(ii);

                        if(child_value->GetName() == wxT("child"))
                        {
                            wxString name;
                            if(!Lookup(*child_value, wxT("exp"), name))
                                name = wxT("--unknown--");

                            Watch *child = new Watch(name);
                            bool has_type;

                            int child_count = ParseSingle(*child, *child_value, has_type);
                            switch(child_count)
                            {
                            case -1:
                                error = true;
                                break;
                            case 0:
                                parent_watch->AddChild(child);
                                child = NULL;
                                break;
                            default:
                                if(has_type)
                                {
                                    ExecuteListCommand(*child, NULL);
                                    parent_watch->AddChild(child);

                                    m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - adding child ")
                                                   + child->GetDebugString()
                                                   + wxT(" to ") + parent_watch->GetDebugString());
                                    child = NULL;
                                }
                                else
                                    ExecuteListCommand(*child, parent_watch);

                                finish = false;
                            }

                            child->Destroy();
                        }
                        else
                        {
                            m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - can't find child in ")
                                           + children->GetTupleValueByIndex(ii)->MakeDebugString());
                        }
                    }
                }
                else
                    error = true;
            }
            break;
        }
    }
    else
        error = true;

    if(error)
    {
        m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - error in command: ") + id.ToString());
        Finish();
    }
    else if(m_sub_commands_left == 0)
    {
        m_logger.Debug(wxT("WatchCreateAction::Output - finishing at") + id.ToString());
        Finish();
    }
}

void WatchCreateAction::OnStart()
{
    wxString symbol;
    m_watch->GetSymbol(symbol);
    Execute(wxString::Format(wxT("-var-create - * %s"), symbol.c_str()));
    m_sub_commands_left = 1;
}

WatchesUpdateAction::WatchesUpdateAction(WatchesContainer &watches, Logger &logger) :
    m_watches(watches),
    m_logger(logger)
{
}

WatchesUpdateAction::~WatchesUpdateAction()
{
    Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->UpdateWatches();
}

void WatchesUpdateAction::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    if(result.GetResultClass() == ResultParser::ClassError)
    {
        Finish();
        return;
    }
    ResultValue const *list = result.GetResultValue().GetTupleValue(wxT("changelist"));
    if(list)
    {
        int count = list->GetTupleSize();
        for(int ii = 0; ii < count; ++ii)
        {
            ResultValue const *value = list->GetTupleValueByIndex(ii);

            wxString expression;
            if(!Lookup(*value, wxT("name"), expression))
            {
                m_logger.Debug(wxT("WatchesUpdateAction::Output - no name in ") + value->MakeDebugString());
                continue;
            }

            Watch *watch = FindWatch(expression, m_watches);
            if(!watch)
            {
                m_logger.Debug(wxT("WatchesUpdateAction::Output - can't find watch ") + expression);
                continue;
            }

            wxString str;
            if(Lookup(*value, wxT("value"), str))
            {
                watch->SetValue(str);
                watch->MarkAsChanged(true);
                m_logger.Debug(wxT("WatchesUpdateAction::Output - ") + expression + wxT(" = ") + str);
            }
        }
    }

    Finish();
}

void WatchesUpdateAction::OnStart()
{
    Execute(wxT("-var-update 2 *"));
}

} // namespace dbg_mi
