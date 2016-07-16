
###################################################
######################## types ####################
###################################################

abstract Model
abstract Action
immutable LeftArc  <: Action end
immutable RightArc <: Action end
immutable Shift    <: Action end

int(::Type{LeftArc})  = 1
int(::Type{RightArc}) = 2
int(::Type{Shift})    = 3
act(i::Int) = i == 1 ? LeftArc  :
              i == 2 ? RightArc :
              i == 3 ? Shift    :
                throw("NO SUCH AN ACTION")

typealias Head Int # token index
typealias Dir Int  # Left, Right, Head
typealias Order Int # Nth Left, Mth Right ..
const L = 1
const R = 2
const H = 3
const maxdir   = 3
const maxorder = 2
typealias Context Tuple{Head,Dir,Order} # e.g. s0h, n0h2 

context(h::Head, d::Dir, o::Order) =
    (h+1) * maxdir * maxorder + d * maxorder + o

initedges(sent::Sent) =
    fill(-1, (length(sent)+1)*maxdir*maxorder+maxdir*maxorder+maxorder)

###################################################
############ State, constructors, IO ##############
###################################################

type State
    step   ::Int
    score  ::Float
    stack  ::Vector{Int}
    buffer ::Int
    edges  ::Vector{Int} #(s0,L,2)->3 means s0lh == 3rd token
    sent   ::Sent
    model  ::Model
    prev   ::State
    prevact::Int
    feat   ::Vector{Int}

    State(step   ::Int,
          score  ::Float,
          stack  ::Vector{Int},
          buffer ::Int,
          edges  ::Vector{Int},
          sent   ::Sent,
          model  ::Model) =
        new(step, score, stack, buffer, edges, sent, model)

    State(step   ::Int,
          score  ::Float,
          stack  ::Vector{Int},
          buffer ::Int,
          edges  ::Vector{Int},
          sent   ::Sent,
          model  ::Model,
          prev   ::State,
          prevact::Int) =
        new(step, score, stack, buffer, edges, sent, model, prev, prevact)
end

function State{M<:Model}(sent::Sent, model::M)
    State(1, zero(Float),  # step, #score
          Int[0],         # stack with root
          1,                       # buffer
          initedges(sent),         # edges
          sent, model)             # sent, model
end

function next{A<:Action}(s::State, action::Type{A})
    State(s.step + 1,                   # step
          s.score + s.model(s, action), #score
          copy(s.stack),                #stack
          s.buffer,                     #buffer
          copy(s.edges),                #edges
          s.sent, s.model, s,            #sent, #model #prev
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
heads(s::State) = map(i->s.edges[context(i,H,1)], 1:length(s.sent))
hashead(s::State, i::Int) = s.edges[context(i,H,1)] != -1

###################################################
#################### "expand"s ####################
###################################################

function expand(s::State, action::Type{LeftArc})
    s   = next(s, action)
    s0i = shift!(s.stack)
    s1i = shift!(s.stack)
    unshift!(s.stack, s0i)
    s.edges[context(s0i,L,2)] = s.edges[context(s0i,L,1)]
    s.edges[context(s1i,H,2)] = s.edges[context(s0i,H,1)]
    s.edges[context(s0i,L,1)] = s1i
    s.edges[context(s1i,H,1)] = s0i
    return s
end

function expand(s::State, action::Type{RightArc})
    s   = next(s, action)
    s0i = shift!(s.stack)
    s1i = s.stack[1]
    s.edges[context(s1i,R,2)] = s.edges[context(s1i,R,1)]
    s.edges[context(s0i,H,2)] = s.edges[context(s1i,H,1)]
    s.edges[context(s1i,R,1)] = s0i
    s.edges[context(s0i,H,1)] = s1i
    return s
end

function expand(s::State, action::Type{Shift})
    s   = next(s, action)
    n0i = s.buffer
    s.buffer += 1
    unshift!(s.stack, n0i)
    return s
end

bufferisempty(s::State) = s.buffer > length(s.sent)

isvalid(s::State, ::Type{LeftArc})  = length(s.stack) >= 2 && s.stack[2] != 0
isvalid(s::State, ::Type{RightArc}) = length(s.stack) >= 2
isvalid(s::State, ::Type{Shift})    = !bufferisempty(s)

isgold(s::State, ::Type{LeftArc})   = isvalid(s, LeftArc)  && s.sent[s.stack[2]].head == s.stack[1]
isgold(s::State, ::Type{RightArc})  = isvalid(s, RightArc) && s.sent[s.stack[1]].head == s.stack[2] && all(i -> s.sent[i].head != first(s.stack), s.buffer:length(s.sent))
isgold(s::State, ::Type{Shift})     = isvalid(s, Shift)

isfinal(s::State) = bufferisempty(s) && length(s.stack) == 1

function expandpred(s::State)
    isfinal(s) && return []
    res = State[]
    isvalid(s, LeftArc)  && push!(res, expand(s, LeftArc))
    isvalid(s, RightArc) && push!(res, expand(s, RightArc))
    isvalid(s, Shift)    && push!(res, expand(s, Shift))
    return res
end

function expandgold(s::State)
    isfinal(s) && return []
    isgold(s, LeftArc)  && return [expand(s, LeftArc)]
    isgold(s, RightArc) && return [expand(s, RightArc)]
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

function maxviolate!(gold::State, pred::State)
    golds = state2array(gold)
    preds = state2array(pred)
    maxv  = typemin(Float); maxk = 1
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
    @printf "UAS: %1.4f\n" uas
end

global const ITERATION = 20

function main()
    info("LOADING SENTENCES")
    trainsents = readconll(trainfile)
    testsents  = readconll(testfile)
    global model = Perceptron(1<<26, 4)

    info("WILL RUN $ITERATION ITERATIONS")
    for i = 1:ITERATION
        info("ITER $i TRAINING")
        progmap(trainsents) do s
            s = State(s, model)
            gold = beamsearch(1, s, expandgold)
            pred = beamsearch(10, s, expandpred)
            maxviolate!(gold, pred)
            pred
        end |> evaluate

        info("ITER $i TESTING")
        progmap(testsents) do s
            pred = State(s, model)
            beamsearch(10, pred, expandpred)
        end |> evaluate
    end
end

###################################################
################## feature function ###############
###################################################

ind2word(s::State, i::Int) = get(s.sent, i, rootword)

function featuregen(s::State)
    stack = s.stack
    stacklen = length(stack)
    s0i = stacklen < 1 ? 0 : stack[1]
    s1i = stacklen < 2 ? 0 : stack[2]
    s2i = stacklen < 3 ? 0 : stack[3]
    n0i = bufferisempty(s) ? 0 : s.buffer
    s0  = ind2word(s, s0i)
    s1  = ind2word(s, s1i)
    s2  = ind2word(s, s2i)
    n0  = ind2word(s, n0i)
    n1  = ind2word(s,  n0i+1)
    s0l  = ind2word(s, s.edges[context(s0i,L,1)])
    s0r  = ind2word(s, s.edges[context(s0i,R,1)])
    s1l  = ind2word(s, s.edges[context(s1i,L,1)])
    s1r  = ind2word(s, s.edges[context(s1i,R,1)])

    len = size(s.model.weights, 1) # used in @template macro
    @template begin
        # template (1)
        (s0.word,)
        (s0.tag,)
        (s0.word, s0.tag)
        (s1.word,)
        (s1.tag,)
        (s1.word, s1.tag)
        (n0.word,)
        (n0.tag,)
        (n0.word, n0.tag)
        
        # additional for (1)
        (n1.word,)
        (n1.tag,)
        (n1.word, n1.tag)
        
        # template (2)
        (s0.word, s1.word)
        (s0.tag, s1.tag)
        (s0.tag, n0.tag)
        (s0.word, s0.tag, s1.tag)
        (s0.tag, s1.word, s1.tag)
        (s0.word, s1.word, s1.tag)
        (s0.word, s0.tag, s1.tag)
        (s0.word, s0.tag, s1.word, s1.tag)
        
        # additional for (2)
        (s0.tag, s1.word)
        (s0.word, s1.tag)
        (s0.word, n0.word)
        (s0.word, n0.tag)
        (s0.tag, n0.word)
        (s1.word, n0.word)
        (s1.tag, n0.word)
        (s1.word, n0.tag)
        (s1.tag, n0.tag)
        
        # template (3)
        (s0.tag, n0.tag, n1.tag)
        (s1.tag, s0.tag, n0.tag)
        (s0.word, n0.tag, n1.tag)
        (s1.tag, s0.word, n0.tag)
        
        # template (4)
        (s1.tag, s1l.tag, s0.tag)
        (s1.tag, s1r.tag, s0.tag)
        (s1.tag, s0.tag, s0r.tag)
        (s1.tag, s1l.tag, s0.tag)
        (s1.tag, s1r.tag, s0.word)
        (s1.tag, s0.word, s0l.tag)
        
        # template (5)
        (s2.tag, s1.tag, s0.tag)
    end
end

