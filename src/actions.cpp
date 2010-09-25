#include "actions.h"

#include <backtracedlg.h>
#include <cbplugin.h>
#include <logmanager.h>
#include <threadsdlg.h>
#include <watchesdlg.h>

#include "cmd_result_parser.h"
#include "frame.h"
#include "updated_variable.h"

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

GenerateBacktrace::GenerateBacktrace(SwitchToFrameInvoker *switch_to_frame, BacktraceContainer &backtrace,
                                     CurrentFrame &current_frame, Logger &logger) :
    m_switch_to_frame(switch_to_frame),
    m_backtrace(backtrace),
    m_logger(logger),
    m_current_frame(current_frame),
    m_first_valid(-1),
    m_parsed_backtrace(false),
    m_parsed_args(false)
{
}

GenerateBacktrace::~GenerateBacktrace()
{
    delete m_switch_to_frame;
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
                    if(s.IsValid() && m_first_valid == -1)
                        m_first_valid = ii;

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
        if (!m_backtrace.empty())
        {
            int frame = m_current_frame.GetUserSelectedFrame();
            if (frame < 0)
                frame = m_first_valid;

            if (frame != m_current_frame.GetStackFrame())
                m_switch_to_frame->Invoke(m_backtrace[frame].GetNumber());
            m_current_frame.SetFrame(frame);
        }

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
    m_threads.clear();

    int current_thread_id = 0;
    if(!Lookup(result.GetResultValue(), wxT("current-thread-id"), current_thread_id))
    {
        m_logger.Debug(wxT("GenerateThreadsList::OnCommandOutput - no current thread id"));
        return;
    }

    ResultValue const *threads = result.GetResultValue().GetTupleValue(wxT("threads"));
    if(!threads || (threads->GetType() != ResultValue::Tuple && threads->GetType() != ResultValue::Array))
    {
        m_logger.Debug(wxT("GenerateThreadsList::OnCommandOutput - no threads"));
        return;
    }
    int count = threads->GetTupleSize();
    for(int ii = 0; ii < count; ++ii)
    {
        ResultValue const &thread_value = *threads->GetTupleValueByIndex(ii);

        int id;
        if(!Lookup(thread_value, wxT("id"), id))
            continue;

        wxString info;
        if(!Lookup(thread_value, wxT("target-id"), info))
            info = wxEmptyString;

        ResultValue const *frame_value = thread_value.GetTupleValue(wxT("frame"));

        if(frame_value)
        {
            wxString str;

            if(Lookup(*frame_value, wxT("addr"), str))
                info += wxT(" ") + str;
            if(Lookup(*frame_value, wxT("func"), str))
            {
                info += wxT(" ") + str;

                if(FrameArguments::ParseFrame(*frame_value, str))
                    info += wxT("(") + str + wxT(")");
                else
                    info += wxT("()");
            }

            int line;

            if(Lookup(*frame_value, wxT("file"), str) && Lookup(*frame_value, wxT("line"), line))
            {
                info += wxString::Format(wxT(" in %s:%d"), str.c_str(), line);
            }
            else if(Lookup(*frame_value, wxT("from"), str))
                info += wxT(" in ") + str;
        }

        m_threads.push_back(cbThread(id == current_thread_id, id, info));
    }

    Manager::Get()->GetDebuggerManager()->GetThreadsDialog()->Reload();
}

void GenerateThreadsList::OnStart()
{
    Execute(wxT("-thread-info"));
}

void ParseWatchInfo(ResultValue const &value, int &children_count, bool &dynamic_has_more)
{
    dynamic_has_more = false;

    int dynamic, has_more;
    if(Lookup(value, wxT("dynamic"), dynamic) && Lookup(value, wxT("has_more"), has_more))
    {
        if(dynamic == 1 && has_more == 1)
            dynamic_has_more = true;
    }

    if(!Lookup(value, wxT("numchild"), children_count))
        children_count = -1;
}

void ParseWatchValueID(Watch &watch, ResultValue const &value)
{
    wxString s;
    if(Lookup(value, wxT("name"), s))
        watch.SetID(s);

    if(Lookup(value, wxT("value"), s))
        watch.SetValue(s);

    if(Lookup(value, wxT("type"), s))
        watch.SetType(s);
}

bool WatchHasType(ResultValue const &value)
{
    wxString s;
    return Lookup(value, wxT("type"), s);
}

void AppendNullChild(Watch &watch)
{
    watch.AddChild(new Watch(wxT("updating...")));
}

Watch * AddChild(Watch &parent, ResultValue const &child_value, wxString const &symbol, WatchesContainer &watches)
{
    wxString id;
    if(!Lookup(child_value, wxT("name"), id))
        return NULL;

    Watch *child = FindWatch(id, watches);
    if(child)
    {
        wxString s;
        if(Lookup(child_value, wxT("value"), s))
            child->SetValue(s);

        if(Lookup(child_value, wxT("type"), s))
            child->SetType(s);
    }
    else
    {
        child = new Watch(symbol);
        ParseWatchValueID(*child, child_value);
        parent.AddChild(child);
    }

    child->MarkAsRemoved(false);
    return child;
}

WatchBaseAction::WatchBaseAction(WatchesContainer &watches, Logger &logger) :
    m_watches(watches),
    m_logger(logger),
    m_sub_commands_left(0)
{
}

WatchBaseAction::~WatchBaseAction()
{
    Manager::Get()->GetDebuggerManager()->GetWatchesDialog()->UpdateWatches();
}

bool WatchBaseAction::ParseListCommand(CommandID const &id, ResultValue const &value)
{
    bool error = false;
    m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - steplistchildren for id: ")
                   + id.ToString() + wxT(" -> ") + value.MakeDebugString());

    ListCommandParentMap::iterator it = m_parent_map.find(id);
    if(it == m_parent_map.end() || !it->second)
    {
        m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - no parent for id: ") + id.ToString());
        return false;
    }


    ResultValue const *children = value.GetTupleValue(wxT("children"));
    if(children)
    {
        int count = children->GetTupleSize();

        m_logger.Debug(wxString::Format(wxT("WatchCreateAction::OnCommandOutput - children %d"), count));
        Watch *parent_watch = it->second;

        for(int ii = 0; ii < count; ++ii)
        {
            ResultValue const *child_value;
            child_value = children->GetTupleValueByIndex(ii);

            if(child_value->GetName() == wxT("child"))
            {
                wxString symbol;
                if(!Lookup(*child_value, wxT("exp"), symbol))
                    symbol = wxT("--unknown--");

                Watch *child = NULL;
                bool has_type, dynamic_has_more;

                int children_count;
                ParseWatchInfo(*child_value, children_count, dynamic_has_more);

                if(dynamic_has_more)
                {
                    child = new Watch(symbol);
                    ParseWatchValueID(*child, *child_value);
                    ExecuteListCommand(*child, parent_watch);
                }
                else
                {
                    switch(children_count)
                    {
                    case -1:
                        error = true;
                        break;
                    case 0:
                        if(!parent_watch->HasBeenExpanded())
                        {
                            parent_watch->SetHasBeenExpanded(true);
                            parent_watch->RemoveChildren();
                        }
                        child = AddChild(*parent_watch, *child_value, symbol, m_watches);
                        child = NULL;
                        break;
                    default:
                        if(WatchHasType(*child_value))
                        {
                            if(!parent_watch->HasBeenExpanded())
                            {
                                parent_watch->SetHasBeenExpanded(true);
                                parent_watch->RemoveChildren();
                            }
                            child = AddChild(*parent_watch, *child_value, symbol, m_watches);
                            AppendNullChild(*child);

                            m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - adding child ")
                                           + child->GetDebugString()
                                           + wxT(" to ") + parent_watch->GetDebugString());
                            child = NULL;
                        }
                        else
                        {
                            wxString id;
                            if(Lookup(*child_value, wxT("name"), id))
                                ExecuteListCommand(id, parent_watch);
                        }
                    }
                }

                child->Destroy();
            }
            else
            {
                m_logger.Debug(wxT("WatchCreateAction::OnCommandOutput - can't find child in ")
                               + children->GetTupleValueByIndex(ii)->MakeDebugString());
            }
        }
        parent_watch->RemoveMarkedChildren();
    }
    else
        error = true;
    return !error;
}

void WatchBaseAction::ExecuteListCommand(Watch &watch, Watch *parent, int start, int end)
{
    CommandID id;

    if(start > -1 && end > -1)
        id = Execute(wxString::Format(wxT("-var-list-children 2 \"%s\" %d %d "), watch.GetID().c_str(), start, end));
    else
        id = Execute(wxString::Format(wxT("-var-list-children 2 \"%s\""), watch.GetID().c_str()));

    m_parent_map[id] = parent ? parent : &watch;
    ++m_sub_commands_left;
}

void WatchBaseAction::ExecuteListCommand(wxString const &watch_id, Watch *parent, int start, int end)
{
    if (!parent)
    {
        m_logger.Debug(wxT("Parent for '") + watch_id + wxT("' is NULL; skipping this watch"));
        return;
    }
    CommandID id;

    if(start > -1 && end > -1)
        id = Execute(wxString::Format(wxT("-var-list-children 2 \"%s\" %d %d "), watch_id.c_str(), start, end));
    else
        id = Execute(wxString::Format(wxT("-var-list-children 2 \"%s\""), watch_id.c_str()));

    m_parent_map[id] = parent;
    ++m_sub_commands_left;
}


WatchCreateAction::WatchCreateAction(Watch::Pointer const &watch, WatchesContainer &watches, Logger &logger) :
    WatchBaseAction(watches, logger),
    m_watch(watch),
    m_step(StepCreate)
{
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
                bool dynamic_has_more;
                int children;
                ParseWatchInfo(value, children, dynamic_has_more);
                ParseWatchValueID(*m_watch, value);
                if(dynamic_has_more)
                {
                    m_step = StepSetRange;
                    Execute(wxT("-var-set-update-range \"") + m_watch->GetID() + wxT("\" 0 100"));
                    AppendNullChild(*m_watch);

                }
                else if(children > 0)
                {
                    m_step = StepListChildren;
                    AppendNullChild(*m_watch);
                }
                else
                    Finish();
            }
            break;
        case StepListChildren:
            error = !ParseListCommand(id, value);
            break;
        }
    }
    else
    {
        if(result.GetResultClass() == ResultParser::ClassError)
            m_watch->SetValue(wxT("the expression can't be evaluated"));
        error = true;
    }

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
    Execute(wxString::Format(wxT("-var-create - @ %s"), symbol.c_str()));
    m_sub_commands_left = 1;
}

WatchesUpdateAction::WatchesUpdateAction(WatchesContainer &watches, Logger &logger) :
    WatchBaseAction(watches, logger)
{
}

void WatchesUpdateAction::OnStart()
{
    m_update_command = Execute(wxT("-var-update 1 *"));
    m_sub_commands_left = 1;
}

bool WatchesUpdateAction::ParseUpdate(ResultParser const &result)
{
    if(result.GetResultClass() == ResultParser::ClassError)
    {
        Finish();
        return false;
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

            UpdatedVariable updated_var;
            if(updated_var.Parse(*value))
            {
                switch(updated_var.GetInScope())
                {
                case UpdatedVariable::InScope_No:
                    watch->Expand(false);
                    watch->RemoveChildren();
                    watch->SetValue(wxT("-- not in scope --"));
                    break;
                case UpdatedVariable::InScope_Invalid:
                    watch->Expand(false);
                    watch->RemoveChildren();
                    watch->SetValue(wxT("-- invalid -- "));
                    break;
                case UpdatedVariable::InScope_Yes:
                    if(updated_var.IsDynamic())
                    {
                        if(updated_var.HasNewNumberOfChildren())
                        {
                            watch->RemoveChildren();

                            if(updated_var.GetNewNumberOfChildren() > 0)
                                ExecuteListCommand(*watch, NULL);
                        }
                        else if(updated_var.HasMore())
                        {
                            watch->MarkChildsAsRemoved(); // watch->RemoveChildren();
                            ExecuteListCommand(*watch, NULL);
                        }
                        else if(updated_var.HasValue())
                        {
                            watch->SetValue(updated_var.GetValue());
                            watch->MarkAsChanged(true);
                        }
                        else
                        {
                            m_logger.Debug(wxT("WatchesUpdateAction::Output - unhandled dynamic variable"));
                            m_logger.Debug(wxT("WatchesUpdateAction::Output - ") + updated_var.MakeDebugString());
                        }
                    }
                    else
                    {
                        if(updated_var.HasNewNumberOfChildren())
                        {
                            watch->RemoveChildren();

                            if(updated_var.GetNewNumberOfChildren() > 0)
                                ExecuteListCommand(*watch, NULL);
                        }
                        if(updated_var.HasValue())
                        {
                            watch->SetValue(updated_var.GetValue());
                            watch->MarkAsChanged(true);
                            m_logger.Debug(wxT("WatchesUpdateAction::Output - ")
                                           + expression + wxT(" = ") + updated_var.GetValue());
                        }
                        else
                        {
                            watch->SetValue(wxEmptyString);
                        }
                    }
                    break;
                }
            }

        }
    }
    return true;
}

void WatchesUpdateAction::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    --m_sub_commands_left;

    if(id == m_update_command)
    {
        for(WatchesContainer::iterator it = m_watches.begin();  it != m_watches.end(); ++it)
            (*it)->MarkAsChangedRecursive(false);

        if(!ParseUpdate(result))
        {
            Finish();
            return;
        }
    }
    else
    {
        ResultValue const &value = result.GetResultValue();
        if(!ParseListCommand(id, value))
        {
            m_logger.Debug(wxT("WatchUpdateAction::Output - ParseListCommand failed ") + id.ToString());
            Finish();
            return;
        }
    }

    if(m_sub_commands_left == 0)
    {
        m_logger.Debug(wxT("WatchUpdateAction::Output - finishing at") + id.ToString());
        Finish();
    }
}

void WatchExpandedAction::OnStart()
{
    ExecuteListCommand(*m_expanded_watch, NULL, 0, 100);
}

void WatchExpandedAction::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    --m_sub_commands_left;
    m_logger.Debug(wxT("WatchExpandedAction::Output - ") + result.GetResultValue().MakeDebugString());
    if(!ParseListCommand(id, result.GetResultValue()))
    {
        m_logger.Debug(wxT("WatchExpandedAction::Output - error in command") + id.ToString());
        Finish();
    }
    else if(m_sub_commands_left == 0)
    {
        m_logger.Debug(wxT("WatchExpandedAction::Output - done"));
        Finish();
    }
}

void WatchCollapseAction::OnStart()
{
    Execute(wxT("-var-delete -c ") + m_collapsed_watch->GetID());
}

void WatchCollapseAction::OnCommandOutput(CommandID const &id, ResultParser const &result)
{
    if(result.GetResultClass() == ResultParser::ClassDone)
    {
        m_collapsed_watch->SetHasBeenExpanded(false);
        m_collapsed_watch->RemoveChildren();
        AppendNullChild(*m_collapsed_watch);
    }
    Finish();
}

} // namespace dbg_mi
