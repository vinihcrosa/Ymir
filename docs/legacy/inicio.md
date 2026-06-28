# Introdução ao repositório

Esse repositório é uma engine de fisica, ela é baseada em um projeto mais antigo, que esta documentado em docs.

Muita coisa vai mudar, portanto não se baseia estritamente no que esta descrito lá.

O objetivo é fazer uma engine de fisica, apenas os calculos, de forma que eu possa pegar essa engine, acoplar com algum Viewer, e ter a visualização

## Alguns requisitos

A engine deve poder ser rodada tanto em linux, macos e windows, e para arquiteturas x86 e ARCH64

A engine deve poder ser portada para uma lib fácil de importar em outros sistemas, uma DLL por exemplo, de forma que eu possa fazer wrappers para outras linguagens, como C#, python e outras, para qualquer OS e Arquitetura

## Stack

A linguagem é C++

A lib para malhas é a CGAL

## Código

O código deve repeitar as melhores práticas de programação, modularidade, principios SOLID, Separação de responsabilidades, Ports and Adapters.

O código deve ser bem documentado, deve usar o doxygen para isso.

O código precisa ter testes de unidade e de integrção, todo código deve ser testado.

