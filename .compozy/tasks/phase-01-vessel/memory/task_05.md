# Task Memory: task_05.md

## Objective Snapshot

Implementar PrescribedController: interpola séries temporais de RPM/ângulo via
`std::lower_bound`, chama `setDemand()` em Thruster/Rudder, erro < 1e-6.
Status: **completed**.

## Important Decisions

- `interpolate()` permanece `private static noexcept` conforme TechSpec.
- `update()` preserva `demandedPitch` e `demandedAzimuth_deg` do thruster ao chamar
  `setDemand(rpm, state.demandedPitch, state.demandedAzimuth_deg)` — PrescribedController
  só controla RPM; azimuth/pitch ficam no estado atual do thruster.
- Não há `std::numbers::pi` em C++17 — substituído por `constexpr double PI` local no test.
- `std::min` em vez de assert/crash para tamanho inconsistente.

## Learnings

- **Thruster e Rudder tinham move constructor = delete**, o que impede `std::vector::emplace_back`
  (o compilador instancia `_M_realloc_insert` mesmo com `reserve()` prévio).
  Correção aplicada: `Thruster(Thruster&&) = default` e `Rudder(Rudder&&) = default`.
  Referência ao config é rebindada para o mesmo objeto (válido). Moved-from continua válido.
  Esta correção é necessária para qualquer código que popule `std::vector<Thruster/Rudder>`.

## Files / Surfaces Touched

- `libs/vessel/include/ymir/vessel/controllers/PrescribedController.h` (novo)
- `libs/vessel/src/controllers/PrescribedController.cpp` (novo)
- `libs/vessel/tests/TestPrescribedController.cpp` (novo, 9 testes)
- `libs/vessel/CMakeLists.txt` — adicionado `src/controllers/PrescribedController.cpp`
- `libs/vessel/tests/CMakeLists.txt` — adicionado `TestPrescribedController.cpp`
- `libs/vessel/include/ymir/vessel/Thruster.h` — move ctor: `= delete` → `= default`
- `libs/vessel/include/ymir/vessel/Rudder.h` — move ctor: `= delete` → `= default`

## Errors / Corrections

- Build falhou com `std::numbers::pi` (C++20). Substituído por `constexpr double PI`.
- Build teria falhado com `std::vector<Thruster>::emplace_back` se move ctor permanecesse
  deletado. Corrigido antes de escrever os testes.

## Ready for Next Run

- PrescribedController pronto para uso em task_08 (DynamicVessel) via `std::variant`.
- Thruster e Rudder agora move-constructible — necessário para DynamicVessel's vector.
