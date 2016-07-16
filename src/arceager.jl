
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
act(i::Int) = i == 1 ? LeftArc  :
              i == 2 ? RightArc :
              i == 3 ? Shift    :
              i == 4 ? Reduce   :
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
    State(1, zero(Float), # step, #score
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
    n0i = s.buffer
    s.edges[context(n0i,L,2)] = s.edges[context(n0i,L,1)]
    s.edges[context(s0i,H,1)] = n0i
    s.edges[context(n0i,L,1)] = s0i
    return s
end

function expand(s::State, action::Type{RightArc})
    s   = next(s, action)
    n0i = s.buffer
    s.buffer += 1
    s0i = first(s.stack)
    unshift!(s.stack, n0i)
    s.edges[context(s0i,R,2)] = s.edges[context(s0i,R,1)]
    s.edges[context(n0i,H,2)] = s.edges[context(s0i,H,1)]
    s.edges[context(s0i,R,1)] = n0i
    s.edges[context(n0i,H,1)] = s0i
    return s
end

function expand(s::State, action::Type{Shift})
    s   = next(s, action)
    n0i = s.buffer
    s.buffer += 1
    unshift!(s.stack, n0i)
    return s
end

function expand(s::State, action::Type{Reduce})
    s = next(s, action)
    shift!(s.stack)
    return s
end

bufferisempty(s::State) = s.buffer > length(s.sent)

isvalid(s::State, ::Type{LeftArc})  = !bufferisempty(s) && !isempty(s.stack) && first(s.stack) != 0 && !hashead(s, first(s.stack))
isvalid(s::State, ::Type{RightArc}) = !bufferisempty(s) && !isempty(s.stack)
isvalid(s::State, ::Type{Shift})    = !bufferisempty(s)
isvalid(s::State, ::Type{Reduce})   = length(s.stack) > 1 && hashead(s, first(s.stack))

isgold(s::State, ::Type{LeftArc})   = isvalid(s, LeftArc)  && first(s.stack) != 0 && s.sent[first(s.stack)].head == s.buffer
isgold(s::State, ::Type{RightArc})  = isvalid(s, RightArc) && s.sent[s.buffer].head == first(s.stack)
isgold(s::State, ::Type{Shift})     = isvalid(s, Shift)    && !bufferisempty(s)
isgold(s::State, ::Type{Reduce})    = isvalid(s, Reduce)   && (bufferisempty(s) || !any(w->w.head == first(s.stack), s.sent[s.buffer:end]))

isfinal(s::State) = bufferisempty(s) && length(s.stack) == 1

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
    s0i = isempty(s.stack) ? 0 : first(s.stack)
    n0i = bufferisempty(s) ? 0 : s.buffer
    s0  = ind2word(s, s0i)
    n0  = ind2word(s, n0i)
    n1  = ind2word(s,  n0i+1)
    n2  = ind2word(s,  n0i+2)
    s0h  = ind2word(s, s.edges[context(s0i,H,1)])
    s0l  = ind2word(s, s.edges[context(s0i,L,1)])
    s0r  = ind2word(s, s.edges[context(s0i,R,1)])
    n0l  = ind2word(s, s.edges[context(n0i,L,1)])
    s0h2 = ind2word(s, s.edges[context(s0i,H,2)])
    s0l2 = ind2word(s, s.edges[context(s0i,L,2)])
    s0r2 = ind2word(s, s.edges[context(s0i,R,2)])
    n0l2 = ind2word(s, s.edges[context(n0i,L,2)])
    s0vr = s.edges[context(s0i,R,2)] != -1 ? 2 : s.edges[context(s0i,R,1)] != -1 ? 1 : 0
    s0vl = s.edges[context(s0i,L,2)] != -1 ? 2 : s.edges[context(s0i,L,1)] != -1 ? 1 : 0
    n0vl = s.edges[context(n0i,L,2)] != -1 ? 2 : s.edges[context(n0i,L,1)] != -1 ? 1 : 0
    distance = s0i != 0 && n0i != 0 ? min(abs(s0i-n0i), 5) : 0

    len = size(s.model.weights, 1) # used in @template macro
    @template begin
    (s0.word,)
    (s0.tag,)
    (s0.word, s0.tag)
    (n0.word,)
    (n0.tag,)
    (n0.word, n0.tag)
    (n1.word,)
    (n1.tag,)
    (n1.word, n1.tag)
    (n2.word,)
    (n2.tag,)
    (n2.word, n2.tag)

    #  from word pairs
    (s0.word, s0.tag, n0.word, n0.tag)
    (s0.word, s0.tag, n0.word)
    (s0.word, n0.word, n0.tag)
    (s0.word, s0.tag, n0.tag)
    (s0.tag, n0.word, n0.tag)
    (s0.word, n0.word)
    (s0.tag, n0.tag)
    (n0.tag, n1.tag)

    #  from three words
    (n0.tag, n1.tag, n2.tag)
    (s0.tag, n0.tag, n1.tag)
    (s0h.tag, s0.tag, n0.tag)
    (s0.tag, s0l.tag, n0.tag)
    (s0.tag, s0r.tag, n0.tag)
    (s0.tag, n0.tag, n0l.tag)

    # distance
    (s0.word, distance)
    (s0.tag, distance)
    (n0.word, distance)
    (n0.tag, distance)
    (s0.word, n0.word, distance)
    (s0.tag, n0.tag, distance)

    #  valency
    (s0.word, s0vr)
    (s0.tag, s0vr)
    (s0.word, s0vl)
    (s0.tag, s0vl)
    (n0.word, n0vl)
    (n0.tag, n0vl)

    #  unigrams
    (s0h.word,)
    (s0h.tag,)
    (s0l.word,)
    (s0l.tag,)
    (s0r.word,)
    (s0r.tag,)
    (n0l.tag,)

    #  third-order
    (s0h2.word,)
    (s0h2.tag,)
    (s0l2.word,)
    (s0l2.tag,)
    (s0r2.word,)
    (s0r2.tag,)
    (n0l2.word,)
    (n0l2.tag,)
    (s0.tag, s0l.tag, s0l2.tag)
    (s0.tag, s0r.tag, s0r2.tag)
    (s0.tag, s0h.tag, s0h2.tag)
    (n0.tag, n0l.tag, n0l2.tag)
    end
end
