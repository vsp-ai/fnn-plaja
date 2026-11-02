set(PA_SMT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/model_z3_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/model_z3_pa.h
    ${CMAKE_CURRENT_LIST_DIR}/pb_query_builder.cpp ${CMAKE_CURRENT_LIST_DIR}/pb_query_builder.h
    ${CMAKE_CURRENT_LIST_DIR}/pb_engine.cpp ${CMAKE_CURRENT_LIST_DIR}/pb_engine.h
    ${CMAKE_CURRENT_LIST_DIR}/state_condition_checker_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/state_condition_checker_pa.h
    ${CMAKE_CURRENT_LIST_DIR}/model_marabou_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/model_marabou_pa.h
    ${CMAKE_CURRENT_LIST_DIR}/action_op_marabou_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/action_op_marabou_pa.h
    ${CMAKE_CURRENT_LIST_DIR}/successor_generator_marabou_pa.cpp ${CMAKE_CURRENT_LIST_DIR}/successor_generator_marabou_pa.h
)
