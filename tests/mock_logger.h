#ifndef _GDB_MI_TESTS_MOCK_LOGGER_H_
#define _GDB_MI_TESTS_MOCK_LOGGER_H_


class MockLogger : public dbg_mi::Logger
{
public:
    virtual void Debug(wxString const &line, Line::Type type)
    {
        Line l;
        l.line = line;
        l.type = type;
        m_debug.push_back(l);
    }
    virtual Line const * GetDebugLine(int index) const
    {
        if(index < static_cast<int>(m_debug.size()))
            return &m_debug[index];
        else
            return &m_empty_line;
    }

    virtual void AddCommand(wxString const &command)
    {
        m_commands_history.push_back(command);
    }

    virtual int GetCommandCount() const
    {
        return m_commands_history.size();
    }
    virtual wxString const& GetCommand(int index) const
    {
        Commands::const_iterator it = m_commands_history.begin();
        std::advance(it, index);
        return *it;
    }

    virtual void ClearCommand()
    {
        m_commands_history.clear();
    }
public:
    int GetDebugLineCount() const { return m_debug.size(); }
private:
    typedef std::vector<Line> Lines;
    typedef std::vector<wxString> Commands;
    Lines m_debug;
    Commands m_commands_history;
    Line m_empty_line;
};


#endif // _GDB_MI_TESTS_MOCK_LOGGER_H_

