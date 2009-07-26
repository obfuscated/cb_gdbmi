#ifndef _DEBUGGER_MI_GDB_CMD_RESULT_PARSER_H_
#define _DEBUGGER_MI_GDB_CMD_RESULT_PARSER_H_

#include <deque>
#include <map>
#include <vector>
#include <wx/string.h>

namespace dbg_mi
{

class ResultValue
{
    typedef std::map<wxString, ResultValue*> TupleType;
public:
    typedef std::deque<ResultValue*> Container;
    enum Type
    {
        Simple = 0,
//        Array,
        Tuple
    };
public:
    void SetName(wxString const &name) { m_name = name; }
    void SetSimpleValue(wxString const &value) { m_value.simple = value; }


    Type GetType() const { return m_type; }
    void SetType(Type type) { m_type = type; }
    wxString const & GetName() const { return m_name; }

    wxString const & GetSimpleValue() const { assert(m_type == Simple); return m_value.simple; }

    int GetTupleSize() const { assert(m_type == Tuple); return m_value.tuple.size(); }
    void SetTupleValue(ResultValue *value)
    {
        TupleType::iterator it = m_value.tuple.find(value->GetName());
        if(it != m_value.tuple.end())
        {
            delete it->second;
            it->second = value;
        }
        else
            m_value.tuple[value->GetName()] = value;
    }
    ResultValue const * GetTupleValue(wxString const &key) const
    {
        assert(m_type == Tuple);
        TupleType::const_iterator it = m_value.tuple.find(key);
        if(it != m_value.tuple.end())
        {
            return it->second;
        }
        else
            return NULL;
    }
    ResultValue const* GetTupleValueByIndex(int index) const
    {
        TupleType::const_iterator it = m_value.tuple.begin();

        std::advance(it, index);
        if(it != m_value.tuple.end())
            return it->second;
        else
            return NULL;
    }

    wxString MakeDebugString() const;

public:
    wxString raw_value;
private:
    wxString m_name;
    Type     m_type;

    struct Value
    {
        wxString simple;
//        std::vector<wxString> array;
        TupleType tuple;
    } m_value;
};

bool ParseValues(wxString const &str, ResultValue::Container &results);
bool ParseValue(wxString const &str, ResultValue &results);

class ResultParser
{
public:
    enum Type
    {
        Result = 0,
        ExecAsyncOutput,
        StatusAsyncOutput,
        NotifyAsyncOutput
    };

    enum Class
    {
        ClassDone = 0,
        ClassRunning,
        ClassConnected,
        ClassError,
        ClassExit,
        ClassStopped
    };

public:
    ~ResultParser();
    bool Parse(wxString const &str, Type result_type);

    wxString MakeDebugString() const;
    Type GetResultType() const { return m_type; }
    Class GetResultClass() const { return m_class; }

    ResultValue::Container const & GetResultValues() const { return m_values; }
private:
    Type m_type;
    Class m_class;
    ResultValue::Container    m_values;
};

} // namespace dbg_mi

#endif // _DEBUGGER_MI_GDB_CMD_RESULT_PARSER_H_
