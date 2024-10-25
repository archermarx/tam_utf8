using Printf

function bytes(str)
    return map(repr, Base.CodeUnits(str))
end

function codepoints(str)
    return map(x -> @sprintf("0x%X", Int(x)), collect(str))
end

function c_arr(x)
    return "{" * join(x, ", ") * "}"
end

function bytes_decl(x, indent = 0)
    indent_str = " "^indent
    println(indent_str * "u8 bytes[] = ", c_arr(bytes(x)), ";")
end

function codepoints_decl(x, indent)
    indent_str = " "^indent
    println(indent_str * "u32 codepoints[] = ", c_arr(codepoints(x)), ";")
end

function decls(x, msg = "")
    println("{")
    indent_size = 4
    bytes_decl(x, indent_size)
    codepoints_decl(x, indent_size)
    indent_str = " "^indent_size
    println(indent_str * "ASSERT_CODEPOINTS(bytes, codepoints, $(msg == "" ? "NULL" : msg));")
    println("}")
end
    
function gen_testfile()
    file = "utf8-test.txt"
    newfile = "utf8-test-bytes.txt"
    buf = UInt8[]
    acc = UInt8[]
    inquotes = false
    test_begun = false
    for c in read(file)
        if inquotes
            if c == Int('\"')
                inquotes = false
                append!(buf, c_arr(map(x -> @sprintf("0x%x", x), acc)))
                acc = UInt8[]
            else
                if !isspace(Char(c)) && Char(c) != '|'
                    push!(acc, c)
                end
            end
        else
            if c == Int('\"')
                inquotes = true
            else
                push!(buf, c)
            end
        end
    end
    write(newfile, String(buf))
end
