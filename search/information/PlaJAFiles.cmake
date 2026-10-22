set(INFORMATION_SOURCES
        # INFORMATION
        ${CMAKE_CURRENT_LIST_DIR}/model_information.cpp ${CMAKE_CURRENT_LIST_DIR}/model_information.h
        ${CMAKE_CURRENT_LIST_DIR}/property_information.cpp ${CMAKE_CURRENT_LIST_DIR}/property_information.h
        ${CMAKE_CURRENT_LIST_DIR}/synchronisation_information.cpp ${CMAKE_CURRENT_LIST_DIR}/synchronisation_information.h

        ${CMAKE_CURRENT_LIST_DIR}/jani_2_interface.h
        ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani_derived.cpp ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani_derived.h
        ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani.cpp ${CMAKE_CURRENT_LIST_DIR}/input_feature_to_jani.h



        # INFORMATION NN INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/using_jani2nnet.h
        ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet.h
        ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet_parser.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2nnet/jani_2_nnet_parser.h

        ${CMAKE_CURRENT_LIST_DIR}/jani2fnn/jani_2_fnn.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2fnn/jani_2_fnn.h
        ${CMAKE_CURRENT_LIST_DIR}/jani2fnn/jani_2_fnn_parser.cpp ${CMAKE_CURRENT_LIST_DIR}/jani2fnn/jani_2_fnn_parser.h
)
