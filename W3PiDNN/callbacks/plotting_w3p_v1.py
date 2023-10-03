import tensorflow as tf
from tensorflow import keras
import matplotlib.pyplot as plt
import os, io

def PLOT_SINGLE(x, xl, lo, hi):
    fig = plt.figure()
    plt.xlabel(xl)
    plt.xlim(lo, hi)
    plt.hist(x, bins=100, range=(lo, hi))
    return fig

def PLOT_DOUBLE(x_s, x_b, xl, lo, hi):
    fig = plt.figure()
    plt.xlabel(xl)
    plt.xlim(lo, hi)
    plt.hist(x_s, bins=100, range=(lo, hi), color='#3371ff', label='signal')
    plt.hist(x_b, bins=100, range=(lo, hi), color='#ffb833', label='background')
    plt.legend(loc='upper center')
    return fig

class w3pPlotCallback(keras.callbacks.Callback):
    def __init__(self, name, data, log_dir):
        super().__init__()
        self.n       = name
        self.x       = data
        self.log_dir = log_dir

        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)

        w3pPlotCallback.writer=tf.summary.create_file_writer(self.log_dir)

    def on_train_end(self, step, log={}):
        w3pPlotCallback.writer.close()
        prediction = self.model.predict(self.x)
        plot_score = PLOT_SINGLE(x=[x[0] for x in prediction], xl='score', lo=-0.5, hi=1.5)
        plot_score.savefig(self.log_dir+"/score{}.pdf".format(self.n.replace('/', '_')))

    def on_epoch_end(self, epoch, logs={}):
        prediction = self.model.predict(self.x)
        plot_score = self.__img_to_tf(PLOT_SINGLE(x=[x[0] for x in prediction], xl='score', lo=-0.5, hi=1.5))

        with w3pPlotCallback.writer.as_default(step=epoch):
            tf.summary.image("score/{}".format(self.n), plot_score, step=epoch)

    @staticmethod
    def __img_to_tf(fig):
        with io.BytesIO() as buffer:
            fig.savefig(buffer, format='png')
            img = tf.image.decode_png(buffer.getvalue(), channels=4)
            img = tf.expand_dims(img, 0)
        return img

class w3pPlotCallback2(keras.callbacks.Callback):
    def __init__(self, name, data_signal, data_background, log_dir):
        super().__init__()
        self.n       = name
        self.x_s     = data_signal
        self.x_b     = data_background
        self.log_dir = log_dir

        if not os.path.exists(self.log_dir):
            os.makedirs(self.log_dir)

        w3pPlotCallback.writer=tf.summary.create_file_writer(self.log_dir)

    def on_train_end(self, step, log={}):
        w3pPlotCallback.writer.close()
        prediction_s = self.model.predict(self.x_s)
        prediction_b = self.model.predict(self.x_b)
        plot_score = PLOT_DOUBLE(x_s=[x[0] for x in prediction_s], x_b=[x[0] for x in prediction_b], xl='score', lo=-0.5, hi=1.5)
        plot_score.savefig(self.log_dir+"/overlayed_score{}.pdf".format(self.n.replace('/', '_')))

    def on_epoch_end(self, epoch, logs={}):
        prediction_s = self.model.predict(self.x_s)
        prediction_b = self.model.predict(self.x_b)
        plot_score = self.__img_to_tf(PLOT_DOUBLE(x_s=[x[0] for x in prediction_s], x_b=[x[0] for x in prediction_b], xl='score', lo=-0.5, hi=1.5))

        with w3pPlotCallback.writer.as_default(step=epoch):
            tf.summary.image("overlayed_score/{}".format(self.n), plot_score, step=epoch)

    @staticmethod
    def __img_to_tf(fig):
        with io.BytesIO() as buffer:
            fig.savefig(buffer, format='png')
            img = tf.image.decode_png(buffer.getvalue(), channels=4)
            img = tf.expand_dims(img, 0)
        return img