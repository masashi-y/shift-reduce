
abstract Model

type Perceptron  <: Model
    weights ::Matrix{Float}
    scores  ::Vector{Float}
end

Perceptron(dim::Int, naction::Int) =
    Perceptron(zeros(Float, dim, naction), zeros(Float, naction))

Base.step(s::Array{Int64, 1}) = 16

# function call{A<:Action}(p::Perceptron, s::State, action::Type{A})
@compat function (p::Perceptron){A<:Action}(s::State, action::Type{A})
    if !isdefined(s, :feat)
        s.feat = featuregen(s)
    end
    res = zero(Float)
    for f in s.feat
        res += p.weights[f,int(action)]
    end
    res
end

function traingold!(p::Perceptron, s::State)
    action = s.prevact
    feat = s.prev.feat
    for f in feat
        p.weights[f, action] += 1.0
    end
end

function trainpred!(p::Perceptron, s::State)
    action = s.prevact
    feat = s.prev.feat
    for f in feat
        p.weights[f, action] -= 1.0
    end
end
