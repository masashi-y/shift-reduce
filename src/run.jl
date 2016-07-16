
# typealias Float Float32
typealias Float Float64
typealias String ASCIIString

using Compat
using ProgressMeter
include("utils.jl")
include("tokens.jl")
# include("arceager.jl")
include("arcstandard.jl")
include("perceptron.jl")

global trainfile = "src/wsj_02-21.conll"
global testfile = "src/wsj_23.conll"

@ConllFormat num word :- tag :- :- head label :- :-

main()
