
using ProgressMeter
include("stack.jl")
include("utils.jl")
include("tokens.jl")
include("arceager.jl")
include("perceptron.jl")

global trainfile = "wsj_02-21.conll"
global testfile = "wsj_23.conll"

@ConllFormat num word :- tag :- :- head label :- :-

main()
