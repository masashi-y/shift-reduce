
typealias String ASCIIString

global words = Dict()
global tags = Dict()
global labels = Dict()
global FORMAT = nothing

immutable Word
    wordstr  ::String
    word     ::Int
    tagstr   ::String
    tag      ::Int
    head     ::Int
    labelstr ::String
    label    ::Int
    feature
    function Word(word, tag, head, label, feature=nothing)
        word_id  = get!(words, word, length(words))
        tag_id   = get!(tags, tag, length(tags))
        label_id = get!(labels, label, length(labels))
        new(word, word_id, tag, tag_id, head, label, label_id, feature)
    end
end

typealias Sent Vector{Word}

rootword = Word("root", "", 0, "")

import Base.(==)
(==)(w1::Word, w2::Word) = w1.word == w2.word

heads(s::Vector{Word}) = map(s->s.head, s)

function Word(line::String)
    line     = line |> chomp |> split
    word     = line[FORMAT["word"]]
    tag      = line[FORMAT["tag"]]
    head     = parse(Int, line[FORMAT["head"]])
    label    = line[FORMAT["label"]]
    Word(word, tag, head, label)
end

Base.print(io::IO, w::Word) =
    print(io, join([w.wordstr, "_", w.tagstr,
      w.tagstr, "_", w.head, w.labelstr, "_", "_"], "\t"))

function Base.print(io::IO, sent::Sent)
    for i = 1:length(sent)
        println(io, i, "\t", sent[i])
    end
end

function readconll(filename::String)
    res = Sent[[]]
    for line in open(readlines, filename)
        if length(line) < 3
            push!(res, []); continue
        end
        push!(res[end], Word(line))
    end
    res
end

macro ConllFormat(args...)
    i = 0
    res = Dict{String,Int}()
    while length(args) > i
        i += 1
        args[i] == :(:-) && continue
        res[string(args[i])] = i
    end
    quote global FORMAT = $(esc(res)) end
end

function dump(filename::String, sents::Vector{Sent})
    res = open(filename, "w")
    for sent in sents
        println(res, sent)
    end
    close(res)
end
