INPUT:
        LIL msg
        LIA 0
        TAH
mainloop:
        LDH
        OUT
        INL
        SUI 0
        JNZ mainloop
        HLT
msg:
        .str "Hello, World!"