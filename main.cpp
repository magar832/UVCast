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
    window.setWindowTitle("UVCast - UVC Capture");
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

    // Video preview
    QVideoWidget *videoWidget = new QVideoWidget;

    // Capture session (video)
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

    // Initial audio
    setupAudioPipeline(audioList.value(0));

    // Handlers
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

    // Layout
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addLayout(controls);
    layout->addWidget(videoWidget);
    window.setLayout(layout);

    window.show();
    return app.exec();
}

