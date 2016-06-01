
abstract Model

type Perceptron  <: Model
    weights ::Matrix{Float32}
    scores  ::Vector{Float32}
end

Perceptron(dim::Int, naction::Int) =
    Perceptron(zeros(Float32, dim, naction), zeros(Float32, naction))

function call{A<:Action}(p::Perceptron, s::State, action::Type{A})
    if !isdefined(s, :feat)
        s.feat = featuregen(s)
    end
    res = 0f0
    for f in s.feat
        @inbounds res += p.weights[f,int(action)]
    end
    res
end

function traingold!(p::Perceptron, s::State)
    action = s.prevact
    feat = s.prev.feat
    for f in feat
        p.weights[f, action] += 1.0f0
    end
end

function trainpred!(p::Perceptron, s::State)
    action = s.prevact
    feat = s.prev.feat
    for f in feat
        p.weights[f, action] -= 1.0f0
    end
end

ind2word(s::State, i::Int) = get(s.sent, i, rootword)

function featuregen(s::State)
    s0i = isempty(s.stack) ? 0 : first(s.stack)
    n0i = isempty(s.buffer) ? 0 : first(s.buffer)
    s0  = ind2word(s, s0i)
    n0  = ind2word(s, n0i)
    n1  = ind2word(s,  get(s.buffer, 2, 0))
    n2  = ind2word(s,  get(s.buffer, 3, 0))
    s0h  = ind2word(s, s.edges[ζ(s0i,H,1)])
    s0l  = ind2word(s, s.edges[ζ(s0i,L,1)])
    s0r  = ind2word(s, s.edges[ζ(s0i,R,1)])
    n0l  = ind2word(s, s.edges[ζ(n0i,L,1)])
    s0h2 = ind2word(s, s.edges[ζ(s0i,H,2)])
    s0l2 = ind2word(s, s.edges[ζ(s0i,L,2)])
    s0r2 = ind2word(s, s.edges[ζ(s0i,R,2)])
    n0l2 = ind2word(s, s.edges[ζ(n0i,L,2)])
    s0vr = s.edges[ζ(s0i,R,2)] != -1 ? 2 : s.edges[ζ(s0i,R,1)] != -1 ? 1 : 0
    s0vl = s.edges[ζ(s0i,L,2)] != -1 ? 2 : s.edges[ζ(s0i,L,1)] != -1 ? 1 : 0
    n0vl = s.edges[ζ(n0i,L,2)] != -1 ? 2 : s.edges[ζ(n0i,L,1)] != -1 ? 1 : 0
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
