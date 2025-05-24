~w::(GetKeyState("s", "P") && Send("{s up}"), !GetKeyState("w", "P") && Send("{w down}"))
~a::(GetKeyState("d", "P") && Send("{d up}"), !GetKeyState("a", "P") && Send("{a down}"))
~s::(GetKeyState("w", "P") && Send("{w up}"), !GetKeyState("s", "P") && Send("{s down}"))
~d::(GetKeyState("a", "P") && Send("{a up}"), !GetKeyState("d", "P") && Send("{d down}"))

~w up::GetKeyState("s", "P") && Send("{s down}")
~a up::GetKeyState("d", "P") && Send("{d down}")
~s up::GetKeyState("w", "P") && Send("{w down}")
~d up::GetKeyState("a", "P") && Send("{a down}")
