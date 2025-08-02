// Build Instructions:
// 1. Install dependencies:
//      sudo apt update
//      sudo apt install qt6-base-dev qt6-multimedia-dev qt6-multimediawidgets-dev
// 2. In the project directory:
//      qmake6 UVCast.pro
//      make
// 3. Run:
//      export QT_MULTIMEDIA_PREFERRED_BACKEND=gstreamer
//      ./UVCast

// File: main.cpp
#include <QApplication>
#include <QWidget>
#include <QtGlobal>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QCamera>
#include <QCameraDevice>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QAudioSink>
#include <QVideoWidget>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVariant>
#include <QMetaType>
#include <QIODevice>

Q_DECLARE_METATYPE(QCameraDevice)
Q_DECLARE_METATYPE(QAudioDevice)

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("UVCast");
    window.setMinimumSize(800, 600);

    // Control panel
    QHBoxLayout *controls = new QHBoxLayout;
    QComboBox *videoCombo = new QComboBox;
    auto videoList = QMediaDevices::videoInputs();
    for (const auto &dev : videoList)
        videoCombo->addItem(dev.description(), QVariant::fromValue(dev));
    controls->addWidget(videoCombo);

    QComboBox *audioCombo = new QComboBox;
    auto audioList = QMediaDevices::audioInputs();
    for (const auto &dev : audioList)
        audioCombo->addItem(dev.description(), QVariant::fromValue(dev));
    controls->addWidget(audioCombo);

    // Fullscreen toggle button
    QPushButton *fsButton = new QPushButton("Full Screen");
    controls->addWidget(fsButton);

    // Video preview
    QVideoWidget *videoWidget = new QVideoWidget;

    // Capture session (video only)
    QMediaCaptureSession session(&window);
    QCamera *camera = new QCamera(videoList.value(0), &window);
    session.setCamera(camera);
    session.setVideoOutput(videoWidget);
    camera->start();

    // Manual audio pipeline: pull from source, push to sink
    auto setupAudioPipeline = [&](const QAudioDevice &inDev) {
        static QAudioSource *audioSrc = nullptr;
        static QAudioSink   *audioSnk = nullptr;
        static QIODevice    *srcIO   = nullptr;
        static QIODevice    *snkIO   = nullptr;
        // Teardown old
        if (audioSrc) { audioSrc->stop(); delete audioSrc; audioSrc = nullptr; }
        if (audioSnk) { audioSnk->stop(); delete audioSnk; audioSnk = nullptr; }
        // Setup new
        QAudioFormat fmt = inDev.preferredFormat();
        QAudioDevice outDev = QMediaDevices::defaultAudioOutput();
        audioSrc = new QAudioSource(inDev, fmt, &window);
        audioSnk = new QAudioSink(outDev, fmt, &window);
        srcIO = audioSrc->start();
        snkIO = audioSnk->start();
        QObject::connect(srcIO, &QIODevice::readyRead, [&](void){
            QByteArray data = srcIO->readAll();
            snkIO->write(data);
        });
    };

    // Initial audio setup
    setupAudioPipeline(audioList.value(0));

    // Connect fullscreen toggle
    QObject::connect(fsButton, &QPushButton::clicked, [&](bool){
        bool fs = videoWidget->isFullScreen();
        videoWidget->setFullScreen(!fs);
    });

    // Handlers for dynamic device switching
    QObject::connect(videoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [&](int){
        camera->stop(); delete camera;
        auto dev = videoCombo->currentData().value<QCameraDevice>();
        camera = new QCamera(dev, &window);
        session.setCamera(camera);
        camera->start();
    });

    QObject::connect(audioCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [&](int){
        auto dev = audioCombo->currentData().value<QAudioDevice>();
        setupAudioPipeline(dev);
    });

    // Layout assembly
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addLayout(controls);
    layout->addWidget(videoWidget);
    window.setLayout(layout);

    window.show();
    return app.exec();
}

