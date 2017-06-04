#ifndef PIXITEM_H
#define PIXITEM_H

#include <QGraphicsItem>
#include <QPixmap>
#include <QPainter>
#include <QRectF>
#include <QMouseEvent>
#include <QPointF>
#include <QDragEnterEvent>
#include <QGraphicsSceneWheelEvent>

enum Enum_ZoomState{
    NO_STATE,
    RESET,
    ZOOM_IN,
    ZOOM_OUT
};

enum Enum_ZoomTimes{
    ZOOM_IN_TIMES = 5,
    ZOOM_OUT_TIMES = -5,
};

class PixItem : public QGraphicsItem        //继承自图元类,实现自定义的图元，，，qt预置的有直线，椭圆，文本图元，矩形图元等
{
public:
    PixItem(QPixmap *pixmap);   //构造函数初始化了变量pix
    QRectF boundingRect() const;    //实现自己的boundingRect 图元边界方法，完成以图元坐标系为基础增加两个像素点的冗余的工作
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget); //重画图形函数
    void wheelEvent(QGraphicsSceneWheelEvent *event);
    void setZoomState(const int &zoomState);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    int getScaleValue() const;
    void setScaleValue(const int &);

private:
    qreal m_scaleValue;   //缩放值

    QPixmap pix;    //作为图元显示的图片
    int m_zoomState;

    bool m_isMove;
    QPointF m_startPos;
};

#endif // PIXITEM_H
