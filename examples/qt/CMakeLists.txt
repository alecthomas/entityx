set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Gui REQUIRED)
find_package(Qt5Quick REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_DECLARATIVE_DEBUG -DQT_QML_DEBUG ")

set(QML_FILES
    main.qml
    Particle.qml
    )

add_executable(qt_example
    example.cc
    qml.qrc
    ${QML_FILES} # So they show up in QtCreator
    )

target_link_libraries(qt_example Qt5::Gui Qt5::Quick)
