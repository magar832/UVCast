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
#include <QLabel>
#include <QVariant>
#include <QMetaType>
#include <QIODevice>
#include <QKeyEvent>
#include <QMessageBox>
#include <QWindow>

Q_DECLARE_METATYPE(QCameraDevice)
Q_DECLARE_METATYPE(QAudioDevice)

class AudioBridge {
public:
    AudioBridge(QObject *parent = nullptr) : parent(parent) {}
    ~AudioBridge() { teardown(); }

    void setup(const QAudioDevice &inDev) {
        teardown();
        if (!inDev.isNull()) {
            QAudioFormat fmt = inDev.preferredFormat();
            QAudioDevice outDev = QMediaDevices::defaultAudioOutput();
            audioSrc = new QAudioSource(inDev, fmt, parent);
            audioSnk = new QAudioSink(outDev, fmt, parent);
            srcIO = audioSrc->start();
            snkIO = audioSnk->start();
            QObject::connect(srcIO, &QIODevice::readyRead, [=] {
                QByteArray data = srcIO->readAll();
                snkIO->write(data);
            });
        }
    }

private:
    void teardown() {
        if (audioSrc) { audioSrc->stop(); delete audioSrc; audioSrc = nullptr; }
        if (audioSnk) { audioSnk->stop(); delete audioSnk; audioSnk = nullptr; }
        srcIO = nullptr;
        snkIO = nullptr;
    }

    QObject *parent;
    QAudioSource *audioSrc = nullptr;
    QAudioSink *audioSnk = nullptr;
    QIODevice *srcIO = nullptr;
    QIODevice *snkIO = nullptr;
};

// Custom widget to handle Esc key for fullscreen exit
class VideoWidgetWithEsc : public QVideoWidget {
public:
    using QVideoWidget::QVideoWidget;

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape && isFullScreen()) {
            setFullScreen(false);
    
            if (QWidget *parent = parentWidget()) {
                if (QLayout *layout = parent->layout()) {
                    layout->removeWidget(this);
                    layout->addWidget(this);  // re-add to ensure reintegration
                    updateGeometry();
                    parent->updateGeometry();
                    layout->activate();
		    QApplication::processEvents();
                    parent->update();
                    this->show();  // make sure it's visible
                }
            }
    
            event->accept();
        } else {
            QVideoWidget::keyPressEvent(event);
        }
    }
};


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("UVCast");
    window.setMinimumSize(800, 600);

    // Control panel
    QHBoxLayout *controls = new QHBoxLayout;
    controls->addWidget(new QLabel("Video Device:"));
    QComboBox *videoCombo = new QComboBox;
    controls->addWidget(videoCombo);

    controls->addWidget(new QLabel("Audio Device:"));
    QComboBox *audioCombo = new QComboBox;
    controls->addWidget(audioCombo);

    QPushButton *fsButton = new QPushButton("Full Screen");
    controls->addWidget(fsButton);

    // Video preview
    VideoWidgetWithEsc *videoWidget = new VideoWidgetWithEsc;

    // Get available devices
    auto videoList = QMediaDevices::videoInputs();
    auto audioList = QMediaDevices::audioInputs();

    for (const auto &dev : videoList)
        videoCombo->addItem(dev.description(), QVariant::fromValue(dev));

    for (const auto &dev : audioList)
        audioCombo->addItem(dev.description(), QVariant::fromValue(dev));

    // Check for devices
    if (videoList.isEmpty()) {
        QMessageBox::critical(nullptr, "No Camera", "No video input devices found.");
        return 1;
    }
    if (audioList.isEmpty()) {
        QMessageBox::critical(nullptr, "No Audio", "No audio input devices found.");
        return 1;
    }

    // Capture session
    QMediaCaptureSession session(&window);
    QCamera *camera = new QCamera(videoList.value(0), &window);
    session.setCamera(camera);
    session.setVideoOutput(videoWidget);
    camera->start();

    // Audio bridge
    AudioBridge audioBridge(&window);
    audioBridge.setup(audioList.value(0));

    // Fullscreen toggle
    QObject::connect(fsButton, &QPushButton::clicked, [&] {
      if (!videoWidget->isFullScreen()) {
          // Find the screen the main window is on
          QScreen *targetScreen = window.screen();
          if (targetScreen && videoWidget->windowHandle()) {
              videoWidget->windowHandle()->setScreen(targetScreen);  // Qt6-safe
          }
  
          videoWidget->setFullScreen(true);
          videoWidget->setFocus();  // for Esc key
      } else {
          videoWidget->setFullScreen(false);
          videoWidget->updateGeometry();
          if (window.layout()) window.layout()->activate();
      }
    });

 
    // Device switching
    QObject::connect(videoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) {
        camera->stop(); delete camera;
        auto dev = videoCombo->currentData().value<QCameraDevice>();
        camera = new QCamera(dev, &window);
        session.setCamera(camera);
        camera->start();
    });

    QObject::connect(audioCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) {
        auto dev = audioCombo->currentData().value<QAudioDevice>();
        audioBridge.setup(dev);
    });

    // Layout
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addLayout(controls);
    layout->addWidget(videoWidget);
    window.setLayout(layout);

    window.show();
    return app.exec();
}

