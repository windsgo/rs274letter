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

Other than the traditional `gt` `lt` `eq` `ne` `ge` `le`,
we alse support `>` `<` `>=` `<=` `==` `!=` in `[...]` expressions.


### if, while, do-while, func

- if

    I use a cleaner way to write if statements, with no explictly o-words,
    and of course, do not support o-word if-statement.
    
    I think that's useless and very complex.

    ```
    if [ #2 > 1 ] 
        G01 X1
    elseif [ #2 < -1 ]
        G01 X-1
    else
        G01 Y1
    endif
    ```