~a::
{
    if GetKeyState("d", "P")
    {
        Send "{d up}"
    }
    if !GetKeyState("a", "P")
    {
        Send "{a down}"
    }
}

~d::
{
    if GetKeyState("a", "P")
    {
        Send "{a up}"
    }
    if !GetKeyState("d", "P")
    {
        Send "{d down}"
    }
}

~w::
{
    if GetKeyState("s", "P")
    {
        Send "{s up}"
    }
    if !GetKeyState("w", "P")
    {
        Send "{w down}"
    }
}

~s::
{
    if GetKeyState("w", "P")
    {
        Send "{w up}"
    }
    if !GetKeyState("s", "P")
    {
        Send "{s down}"
    }
}

~a up::
{
    if GetKeyState("d", "P")
    {
        Send "{d down}"
    }
}

~d up::
{
    if GetKeyState("a", "P")
    {
        Send "{a down}"
    }
}

~w up::
{
    if GetKeyState("s", "P")
    {
        Send "{s down}"
    }
}

~s up::
{
    if GetKeyState("w", "P")
    {
        Send "{w down}"
    }
}