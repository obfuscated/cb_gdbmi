#include "cmd_result_parser.h"

#include "cmd_result_tokens.h"

namespace dbg_mi
{

wxString TrimmedSubString(wxString const &str, int start, int length)
{
    int real_start = start;
    int real_end = start + length - 1;

    for(int ii = real_start; ii < real_end; ++ii)
    {
        if(str[ii] == _T(' ') || str[ii] == _T('\t'))
            ++real_start;
        else
            break;
    }

    for(int ii = real_end; ii >= real_start; --ii)
    {
        if(str[ii] == _T(' ') || str[ii] == _T('\t'))
            --real_end;
        else
            break;
    }

    return str.substr(real_start, real_end - real_start + 1);
}

bool ParseTuple(wxString const &str, int &start, ResultValue &tuple)
{
    Token token;
    int pos = start;
    ResultValue *curr_value = NULL;

    enum Step
    {
        Nothing,
        Name,
        Equal,
        Value
    };
    Step step = Nothing;

    while(pos < static_cast<int>(str.length()))
    {
        if(!GetNextToken(str, pos, token))
            return false;

        switch(token.type)
        {
        case Token::String:
            // set the value
            if(curr_value)
            {
                if(step != Equal)
                    return false;
                step = Value;

                curr_value->SetType(ResultValue::Simple);
                curr_value->SetSimpleValue(token.ExtractString(str));
            }
            else
            {
                if(step != Nothing)
                    return false;
                step = Name;

                curr_value = new ResultValue;
                curr_value->SetName(token.ExtractString(str));
            }
            break;

        case Token::Equal:
            if(!curr_value)
                return false;
            if(step != Name)
            {
                delete curr_value;
                return false;
            }
            step = Equal;
            break;

        case Token::Comma:
            if(!curr_value)
                return false;
            if(tuple.GetType() == ResultValue::Array)
            {
                if(step == Name)
                {
                    curr_value->SetSimpleValue(curr_value->GetName());
                    curr_value->SetName(_T(""));
                }
                else if(step != Value)
                {
                    delete curr_value;
                    return false;
                }
            }
            else
            {
                if(step != Value)
                {
                    delete curr_value;
                    return false;
                }
            }
            tuple.SetTupleValue(curr_value);
            curr_value = NULL;
            step = Nothing;
            break;

        case Token::TupleStart:
            if(!curr_value)
                return false;
            if(step != Equal)
            {
                delete curr_value;
                return false;
            }

            curr_value->SetType(ResultValue::Tuple);
            pos = token.end;
            if(!ParseTuple(str, pos, *curr_value))
            {
                delete curr_value;
                return false;
            }
            else
            {
                token.end = pos;
                step = Value;
            }
            break;

        case Token::ListStart:
            if(!curr_value)
                return false;
            if(step != Equal)
            {
                delete curr_value;
                return false;
            }
            curr_value->SetType(ResultValue::Array);
            pos = token.end;
            if(!ParseTuple(str, pos, *curr_value))
            {
                delete curr_value;
                return false;
            }
            else
            {
                token.end = pos;
                step = Value;
            }
            break;

        case Token::TupleEnd:
        case Token::ListEnd:
            if(!curr_value)
                return false;
            else
            {
                if(tuple.GetType() == ResultValue::Array)
                {
                    if(step == Name)
                    {
                        curr_value->SetSimpleValue(curr_value->GetName());
                        curr_value->SetName(_T(""));
                    }
                    else if(step != Value)
                    {
                        delete curr_value;
                        return false;
                    }
                }
                else if(step != Value)
                {
                    delete curr_value;
                    return false;
                }
                start = pos + 1;
                tuple.SetTupleValue(curr_value);
                return true;
            }
            break;
        }

        pos = token.end;
    }

    if(curr_value)
        tuple.SetTupleValue(curr_value);
    start = pos;
    return true;
}


bool ParseValue(wxString const &str, ResultValue &results)
{
    results.SetType(ResultValue::Tuple);
    int start = 0;
    return ParseTuple(str, start, results);
}

wxString ResultValue::MakeDebugString() const
{
    switch(m_type)
    {
    case Simple:
        if(m_name.empty())
            return m_value.simple;
        else
            return m_name + _T("=") + m_value.simple;
        break;
    case Tuple:
        {
            wxString s;
            if(m_name.empty())
                s = _T("{");
            else
                s = m_name + _T("={");

            bool first = true;
            for(Container::const_iterator it = m_value.tuple.begin(); it != m_value.tuple.end(); ++it)
            {
                if(first)
                    first = false;
                else
                    s += _T(",");

                s += (*it)->MakeDebugString();
            }

            s += _T("}");
            return s;
        }
    case Array:
        {
            wxString s;
            if(m_name.empty())
                s = _T("[");
            else
                s = m_name + _T("=[");
            bool first = true;
            for(Container::const_iterator it = m_value.tuple.begin(); it != m_value.tuple.end(); ++it)
            {
                if(first)
                    first = false;
                else
                    s += _T(",");

                s += (*it)->MakeDebugString();
            }

            s += _T("]");
            return s;
        }
    default:
        return _T("not_initialized");
    }
}

ResultParser::~ResultParser()
{
    for(ResultValue::Container::iterator it = m_values.begin(); it != m_values.end(); ++it)
        delete *it;
}

bool ResultParser::Parse(wxString const &str, Type result_type)
{
    m_type = result_type;

    int after_class_index = 0;

    if(str.StartsWith(_T("done")))
    {
        m_class = ClassDone;
        after_class_index = 4;
    }
    else if(str.StartsWith(_T("stopped")))
    {
        m_class = ClassStopped;
        after_class_index = 7;
    }
    else if(str.StartsWith(_T("running")))
    {
        m_class = ClassRunning;
        after_class_index = 7;
    }
    else if(str.StartsWith(_T("connected")))
    {
        m_class = ClassConnected;
        after_class_index = 9;
    }
    else if(str.StartsWith(_T("error")))
    {
        m_class = ClassError;
        after_class_index = 5;
    }
    else if(str.StartsWith(_T("exit")))
    {
        m_class = ClassExit;
        after_class_index = 4;
    }
    else
        return false;

    if(str[after_class_index] != _T(','))
        return false;

    int brace_count = 0;
    size_t start = after_class_index + 1;
    bool quoted_started = false;
    for(size_t ii = start; ii < str.length(); ++ii)
    {
        wxChar ch = str[ii];

        if(ch == _T('"'))
        {
            quoted_started = !quoted_started;
            continue;
        }

        if(quoted_started)
            continue;

        switch(ch)
        {
        case _T(','):
            if(brace_count == 0)
            {
                ResultValue *v = new ResultValue;
                v->raw_value = str.substr(start, ii - start);
                m_values.push_back(v);
                start = ii + 1;
            }
            break;
        case _T('['):
        case _T('{'):
            ++brace_count;
            break;
        case _T(']'):
        case _T('}'):
            --brace_count;
            break;
        }
    }

    if(start < str.length())
    {
        ResultValue *v = new ResultValue;
        v->raw_value = str.substr(start, str.length() - start);
        m_values.push_back(v);
    }

    return true;
}

wxString ResultParser::MakeDebugString() const
{
    wxString s(_T("type: "));
    switch(m_type)
    {
    case Result:
        s += _T("result");
        break;
    case ExecAsyncOutput:
        s += _T("exec-async-ouput");
        break;
    case StatusAsyncOutput:
        s += _T("status-async-ouput");
        break;
    case NotifyAsyncOutput:
        s += _T("notify-async-ouput");
        break;
    default:
        s += _T("unknown");
    }
    s += _T("\nclass: ");
    switch(m_class)
    {
    case ClassDone:
        s += _T("done");
        break;
    case ClassRunning:
        s += _T("running");
        break;
    case ClassConnected:
        s += _T("connected");
        break;
    case ClassError:
        s += _T("error");
        break;
    case ClassExit:
        s += _T("exit");
        break;
    case ClassStopped:
        s += _T("stopped");
        break;
    default:
        s += _T("unknown");
    }

    if(!m_values.empty())
    {
        s += _T("\nresults:\n");
        int ii = 0;
        for(ResultValue::Container::const_iterator it = m_values.begin(); it != m_values.end(); ++it, ++ii)
        {
            s += wxString::Format(_T("[%d] = "), ii);
            s += (*it)->raw_value;
            s += _T("\n");
        }
    }

    return s;
}

} // namespace dbg_mi
