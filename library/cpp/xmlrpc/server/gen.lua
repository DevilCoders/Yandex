local N = tonumber(arg[1])

template1 = [[
template <class F>
struct TCall<%u, F> {
    static TValue Call(F f, const TArray& p) {
        return f(%s);
    }
};
]]

template2 = 'Cast<TFunctionArg<F, %u - 1>>(p.IndexOrNull(%u))'

for i = 0, N - 1 do
    s = ''

    for j = 0, i do
        if s:len() > 0 then
            s = s .. ', '
        end

        s = s .. template2:format(j + 1, j)
    end

    print(template1:format(i + 1, s))
end
