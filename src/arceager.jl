
###################################################
######################## types ####################
###################################################

abstract Model
abstract Action
immutable LeftArc  <: Action end
immutable RightArc <: Action end
immutable Shift    <: Action end
immutable Reduce   <: Action end

int(::Type{LeftArc})  = 1
int(::Type{RightArc}) = 2
int(::Type{Shift})    = 3
int(::Type{Reduce})   = 4
act(i::Int) = i == 1 ? LeftArc :
              i == 2 ? RightArc :
              i == 3 ? Shift :
              i == 4 ? Reduce : throw("NO SUCH AN ACTION")

typealias Head Int # token index
typealias Dir Int  # Left, Right, Head
typealias Order Int # Nth Left, Mth Right ..
const L = 1
const R = 2
const H = 3
const maxdir   = 3
const maxorder = 2
typealias Context Tuple{Head,Dir,Order} # e.g. s0h, n0h2 

ζ(h::Head, d::Dir, o::Order) =
    (h+1) * maxdir * maxorder + d * maxorder + o

macro initedges()
    :( fill(-1, (length(sent)+1)*maxdir*maxorder+maxdir*maxorder+maxorder) )
end

###################################################
############ State, constructors, IO ##############
###################################################

type State
    step   ::Int
    score  ::Float32
    stack  ::Vector{Int}
    buffer ::Vector{Int}
    edges  ::Vector{Int} #(s0,L,2)->3 means s0lh == 3rd token
    sent   ::Sent
    model  ::Model
    prev   ::State
    prevact::Int
    feat   ::Vector{Int}

    State(step   ::Int,
          score  ::Float32,
          stack  ::Vector{Int},
          buffer ::Vector{Int},
          edges  ::Vector{Int},
          sent   ::Sent,
          model  ::Model) =
        new(step, score, stack, buffer, edges, sent, model)

    State(step   ::Int,
          score  ::Float32,
          stack  ::Vector{Int},
          buffer ::Vector{Int},
          edges  ::Vector{Int},
          sent   ::Sent,
          model  ::Model,
          prev   ::State,
          prevact::Int) =
        new(step, score, stack, buffer, edges, sent, model, prev, prevact)
end

function State{M<:Model}(sent::Sent, model::M)
    State(1, 0f0,         #step, #score
          [0],          # stack with root
          collect(1:length(sent)), # buffer
          @initedges,              # edges
          sent, model)             # sent, model
end

function next{A<:Action}(s::State, action::Type{A})
    State(s.step + 1,                   # step
          s.score + s.model(s, action), #score
          copy(s.stack),                #stack
          copy(s.buffer),               #buffer
          copy(s.edges),                #edges
          s.sent, s.model, s,           #sent, #model #prev
          int(action))                  #prevact
end

# prints [ a/NN b/VB ][ c/NN d/PP ]
function Base.print(io::IO, s::State)
    stack = map(s.stack) do i
        i == 0 ? "ROOT/ROOT" :
        s.sent[i].wordstr * "/" * s.sent[i].tagstr
    end |> reverse |> x -> join(x," ")
    buffer = map(s.buffer) do i
        s.sent[i].wordstr * "/" * s.sent[i].tagstr
    end |> x -> join(x, " ")
    print(io, "[", stack, "][", buffer, "]")
end

function stacktrace(io::IO, s::State)
    ss = state2array(s)
    println(io, ss[1])
    for i in 2:length(ss)
        println(io, act(ss[i].prevact))
        println(io, ss[i])
    end
end
stacktrace(s::State) = stacktrace(STDOUT, s)

function toconll(io::IO, s::State)
    pred = heads(s)
    for i in 1:length(s.sent)
        println(io, i, "\t", s.sent[i], "\t", pred[i])
    end
    println(io, "")
end
conll(s::State) = conll(STDOUT, s)

# to retrieve result
heads(s::State) = map(i->s.edges[ζ(i,H,1)], 1:length(s.sent))
hashead(s::State, i::Int) = s.edges[ζ(i,H,1)] != -1

###################################################
#################### "expand"s ####################
###################################################

function expand(s::State, action::Type{LeftArc})
    s = next(s, action)
    s0i = shift!(s.stack)
    n0i = first(s.buffer)
    s.edges[ζ(n0i,L,2)] = s.edges[ζ(n0i,L,1)]
    s.edges[ζ(s0i,H,1)] = n0i
    s.edges[ζ(n0i,L,1)] = s0i
    return s
end

function expand(s::State, action::Type{RightArc})
    s = next(s, action)
    n0i = shift!(s.buffer)
    s0i = first(s.stack)
    unshift!(s.stack, n0i)
    s.edges[ζ(s0i,R,2)] = s.edges[ζ(s0i,R,1)]
    s.edges[ζ(n0i,H,2)] = s.edges[ζ(s0i,H,1)]
    s.edges[ζ(s0i,R,1)] = n0i
    s.edges[ζ(n0i,H,1)] = s0i
    return s
end

function expand(s::State, action::Type{Shift})
    s = next(s, action)
    n0i = shift!(s.buffer)
    unshift!(s.stack, n0i)
    return s
end

function expand(s::State, action::Type{Reduce})
    s = next(s, action)
    shift!(s.stack)
    return s
end

isvalid(s::State, ::Type{LeftArc})  = !isempty(s.buffer) && !isempty(s.stack) && first(s.stack) != 0 && !hashead(s, first(s.stack))
isvalid(s::State, ::Type{RightArc}) = !isempty(s.buffer) && !isempty(s.stack)
isvalid(s::State, ::Type{Shift})    = !isempty(s.buffer)
isvalid(s::State, ::Type{Reduce})   = length(s.stack) > 1 && hashead(s, first(s.stack))
isgold(s::State, ::Type{LeftArc})   = isvalid(s, LeftArc) && first(s.stack) != 0 && s.sent[first(s.stack)].head == first(s.buffer)
isgold(s::State, ::Type{RightArc})  = isvalid(s, RightArc) && s.sent[first(s.buffer)].head == first(s.stack)
isgold(s::State, ::Type{Shift})     = isvalid(s, Shift) && !isempty(s.buffer)
isgold(s::State, ::Type{Reduce})    = isvalid(s, Reduce) && (isempty(s.buffer) || !any(w->w.head == first(s.stack), s.sent[first(s.buffer):end]))
isfinal(s::State) = isempty(s.buffer) && length(s.stack) == 1

function expandpred(s::State)
    isfinal(s) && return []
    res = State[]
    isvalid(s, LeftArc)  && push!(res, expand(s, LeftArc))
    isvalid(s, RightArc) && push!(res, expand(s, RightArc))
    isvalid(s, Shift)    && push!(res, expand(s, Shift))
    isvalid(s, Reduce)   && push!(res, expand(s, Reduce))
    return res
end

function expandgold(s::State)
    isfinal(s) && return []
    isgold(s, LeftArc)  && return [expand(s, LeftArc)]
    isgold(s, RightArc) && return [expand(s, RightArc)]
    isgold(s, Reduce)   && return [expand(s, Reduce)]
    isgold(s, Shift)    && return [expand(s, Shift)]
    stacktrace(s)
    throw("NO ACTION TO PERFORM") 
end

###################################################
################# decoder & trainer ###############
###################################################

st_lessthan(x::State, y::State) = y.score < x.score
function beamsearch(beamsize::Int, initstate::State, expand::Function)
    chart = Vector{State}[[initstate]]
    i = 1
    while i <= length(chart)
        states = chart[i]
        length(states) > beamsize && sort!(states, lt=st_lessthan)
        for j = 1:min(beamsize, length(states))
            for s in expand(states[j])
                while s.step > length(chart) push!(chart, []) end
                push!(chart[s.step], s)
            end
        end
        i += 1
    end
    sort!(chart[end], lt=st_lessthan)
    chart[end][1]
end

function state2array(s::State)
    res = Vector{State}(s.step)
    st = s
    while st.step > 1
        res[st.step] = st
        st = st.prev
    end
    res[st.step] = st
    res
end

function maxviolate!(golds::Vector{State}, preds::Vector{State})
    maxv  = typemin(Float32); maxk = 1
    for k = 2:min(length(golds), length(preds))
        v = preds[k].score - golds[k].score
        if v >= maxv
            maxv, maxk = v, k
        end
    end
    for i = 2:maxk
        traingold!(model, golds[i])
        trainpred!(model, preds[i])
    end
end

###################################################
################# evaluator & main ################
###################################################

function evaluate(ss::Vector{State})
    ignore = ["''", ",", ".", ":", "``", "''"]
    num, den = 0, 0
    for s in ss
        pred = heads(s)
        gold = heads(s.sent)
        @assert length(pred) == length(gold)
        for i in 1:length(pred)
            s.sent[i].wordstr in ignore && continue
            den += 1
            pred[i] == gold[i] && (num += 1)
        end
    end
    uas = float(num) / float(den)
    @sprintf "UAS: %1.4f\n" uas
end

global const ITERATION = 20

function main()
    info("LOADING SENTENCES")
    trainsents = readconll(trainfile)
    testsents  = readconll(testfile)
    global model = Perceptron(1<<26, 4)

    info("CREATING TRAINING SAMPLES")
    goldstates = progmap(trainsents) do s
        gold = State(s, model)
        beamsearch(1, gold, expandgold) |> state2array
    end

    info("WILL RUN $ITERATION ITERATIONS")
    for i = 1:ITERATION
        info("ITER $i TRAINING")
        progmap(shuffle(goldstates)) do golds
            preds = beamsearch(10, golds[1], expandpred) |> state2array
            maxviolate!(golds, preds)
            preds[end]
        end |> evaluate

        info("ITER $i TESTING")
        progmap(testsents) do s
            pred = State(s, model)
            beamsearch(10, pred, expandpred)
        end |> evaluate
    end
end

