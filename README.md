# RS274LETTER

Modern RS274 intepreter, with major difference in flow control, function definiation.

Havn't finished.
Now still in progress.

## More Restrict (different) rules for some grammer in RS274

### Name-Indexed Variable

I'll only support letters(a-z,A-Z) numbers(0-9) and "_" in Name-Indexed Variable,
which is just the regular expressin `\w+` will match.

In name-indexed variable `#<_var_name_>`, I will not try to recognize any nested 
expressions or comments in it, which means like `#<ab(cmts)c>` will not be `#<abc>` 
but just throw exception. Meanwhile, spaces and the lower or upper case will
***NOT*** be omitted inside the angle brackets.

The name-indexed `#<...>` rules is the same as o-words `o<...>`.

### Number-Indexed Variable

I'll allow these types of number-indexed variable.

1. `#` with a direct number literal, such as `#123`
2. `#` with a squre bracket expression, such as `#[1 + 2 * #10]`
3. `#` with a `variable`, such as `##1`, `##<var>`, `##[1+#3]`

Note: the number after `#` will be calced in the examine stage, all these expressions 
directly after `#` should be calced as integer, with a tolerance of less than 0.000001.

> Why do we have a *tolerance*? Because all expressions are calced as double by default.

The number-indexed `#...` rules is the same as o-words `o...`.

### Expression

- Value of assignment expression
  
    In traditional RS274, `#1=[#2=3]` will not pass the syntax examine, but I'll make the
    assignment expression have a value which is equal to the assigned variable. 

    In this case, `#1=[#2=3]` will be parsed as `3` -> `#2`, and then `#2` -> `#1`.

- Expression-statement syntax
    
    RS274 does not allow non-assign expression appears independently.

    In an `expressionStatement`, which appears in a single line, the most outside expression
    of the statement can only be an assignment expression.

    `Parser::assignmentExpression()`  can return just an additive expression without an ASSIGN_OPERATOR. We only need to notice this when we are generating the most outside
    of an `expressionStatement`, so we add a param called `must_be_assignment` to 
    accomplish this. 

    In all other cases, if we are parsing an expression, we'll allow all kind of nested
    expressions.

- Explanation of assign
    
    Like traditional RS274, I'll only support:

    - `#1 = 1` or `#1 = -1`
    - `#1 = #2` or `#1 = -#2`
    - `#1 = []` or `#1 = -[]`

    - `X1` or `X-1`
    - `X#2` or `X-#2`
    - `X[]` or `X-[]`

    Expressions like `#1 = 1 + 1`, this might be meaningful, but we won't allow that,
    just to follow the RS274 general usage.

    I do this in the `primaryExpression` to accomplish this, especially about how to
    deal with the forward ADDITIVE_OPERATOR `+/-`.


### compare operator

Having the traditional `gt` `lt` `eq` `ne` `ge` `le`,
I decided to ***temporarily*** support `>` `<` `>=` `<=` `==` `!=` in `[...]` expressions, because it might conflict the usage of `#<var>`.


### if, while, do-while, sub

- if

    Example:

    ```
    #123 = 1
    o1 if [ #2 > 1 ] 
        G01 X1
    o1 elseif [ #2 < -1 ]
        G01 X-1
    o#123 else
        G01 Y1
    o1 endif
    ```

    Note: we will examine whether these o-words are the same for an if statement,
    but we do not rely on this rule to parse an if statement.

    Note: we may examine whether the numbered index of o-word is unique.

- sub

    #### sub, endsub

    We will ***no longer*** support the M98, M99 call like below:

    ```
    o1
    M98 P100 L2
    M30

    o100
        G01 X1
    M99
    ```

    which I think, is a poorly designed language pattern.

    Instead, we use what is defined in linuxCNC rs274 like below:

    ```
    o100 sub

        o2 if [#1]  (#1 is the argument passed by o100 call [1])
    o100 return     (return to o100 call)
        o2 endif

        G01 X1

    o100 endsub

    o100 call [1]   (call o100 sub with one argument 1)

    ```

    In this pattern, the sub function should be defined **before** the `o-call`,
    however, I might support afterwards define in the implement myself, this 
    is to be done in the future.

    #### call

    Currently not support calling files.

- while

    I ***DO NOT*** support `do-while` structure, only `while-endwhile` structure
    is implemented, for the reason that I hope to identify the code structure without
    the help of o-word value.

    Likewise, the o-break will only break the current loop, you ***CAN NOT*** break the
    outside loop in an inside loop!

    Note: It is tested in the original linuxcnc-sai that the o-break statement will break
    all the loops, while here I hope to only break the current loop, which may seems to be
    more meaningful and correct.

    It strongly recommended not to nest while-loop, and use o-break and o-continue with the 
    closest while-statement o-word.

    ```
    #1 = 10
    G91 F1
    o1 while [#1 lt 10]
    
        G01 X1
        #1 = [#1 + 1]
        
        o2 if [#1 gt 5]

    o1 break

        o2 endif
    
    o1 endwhile
    G90
    M2
    ```

- repeat

    ```
    G91 F1
    o1 repeat [1]
        G01 X1
    o1 endrepeat
    G90
    M2
    ```