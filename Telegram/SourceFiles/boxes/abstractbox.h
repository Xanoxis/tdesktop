/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

#include "layerwidget.h"
#include "ui/widgets/shadow.h"

namespace Ui {
class RoundButton;
class IconButton;
class ScrollArea;
template <typename Widget>
class WidgetFadeWrap;
} // namespace Ui

class BoxLayerTitleShadow : public Ui::PlainShadow {
public:
	BoxLayerTitleShadow(QWidget *parent);

};

class BoxContentDelegate {
public:
	virtual void setLayerType(bool layerType) = 0;
	virtual void setTitle(const QString &title, const QString &additional) = 0;

	virtual void clearButtons() = 0;
	virtual QPointer<Ui::RoundButton> addButton(const QString &text, base::lambda<void()> &&clickCallback, const style::RoundButton &st) = 0;
	virtual QPointer<Ui::RoundButton> addLeftButton(const QString &text, base::lambda<void()> &&clickCallback, const style::RoundButton &st) = 0;
	virtual void updateButtonsPositions() = 0;

	virtual void setDimensions(int newWidth, int maxHeight) = 0;
	virtual void setNoContentMargin(bool noContentMargin) = 0;
	virtual bool isBoxShown() const = 0;
	virtual void closeBox() = 0;

};

class BoxContent : public TWidget, protected base::Subscriber {
	Q_OBJECT

public:
	BoxContent() {
		setAttribute(Qt::WA_OpaquePaintEvent);
	}

	void setDelegate(BoxContentDelegate *newDelegate) {
		_delegate = newDelegate;
		prepare();
		setInnerFocus();
	}
	virtual void setInnerFocus() {
		setFocus();
	}
	virtual void closeHook() {
	}

	bool isBoxShown() const {
		return getDelegate()->isBoxShown();
	}
	void closeBox() {
		getDelegate()->closeBox();
	}

public slots:
	void onScrollToY(int top, int bottom = -1);

	void onDraggingScrollDelta(int delta);

protected:
	virtual void prepare() = 0;

	void setLayerType(bool layerType) {
		getDelegate()->setLayerType(layerType);
	}
	void setTitle(const QString &title, const QString &additional = QString()) {
		getDelegate()->setTitle(title, additional);
	}

	void clearButtons() {
		getDelegate()->clearButtons();
	}
	QPointer<Ui::RoundButton> addButton(const QString &text, base::lambda<void()> &&clickCallback);
	QPointer<Ui::RoundButton> addLeftButton(const QString &text, base::lambda<void()> &&clickCallback);
	QPointer<Ui::RoundButton> addButton(const QString &text, base::lambda<void()> &&clickCallback, const style::RoundButton &st) {
		return getDelegate()->addButton(text, std_::move(clickCallback), st);
	}
	void updateButtonsGeometry() {
		getDelegate()->updateButtonsPositions();
	}

	void setNoContentMargin(bool noContentMargin) {
		if (_noContentMargin != noContentMargin) {
			_noContentMargin = noContentMargin;
			setAttribute(Qt::WA_OpaquePaintEvent, !_noContentMargin);
		}
		getDelegate()->setNoContentMargin(noContentMargin);
	}
	void setDimensions(int newWidth, int maxHeight) {
		getDelegate()->setDimensions(newWidth, maxHeight);
	}
	void setInnerTopSkip(int topSkip, bool scrollBottomFixed = false);

	template <typename Widget>
	QPointer<Widget> setInnerWidget(object_ptr<Widget> inner, const style::ScrollArea &st, int topSkip = 0) {
		auto result = QPointer<Widget>(inner.data());
		setInnerTopSkip(topSkip);
		setInner(std_::move(inner), st);
		return result;
	}

	template <typename Widget>
	QPointer<Widget> setInnerWidget(object_ptr<Widget> inner, int topSkip = 0) {
		auto result = QPointer<Widget>(inner.data());
		setInnerTopSkip(topSkip);
		setInner(std_::move(inner));
		return result;
	}

	template <typename Widget>
	object_ptr<Widget> takeInnerWidget() {
		return static_object_cast<Widget>(doTakeInnerWidget());
	}

	void setInnerVisible(bool scrollAreaVisible);
	QPixmap grabInnerCache();

	void resizeEvent(QResizeEvent *e) override;
	void paintEvent(QPaintEvent *e) override;

private slots:
	void onScroll();
	void onInnerResize();

	void onDraggingScrollTimer();

private:
	void setInner(object_ptr<TWidget> inner);
	void setInner(object_ptr<TWidget> inner, const style::ScrollArea &st);
	void updateScrollAreaGeometry();
	void updateInnerVisibleTopBottom();
	void updateShadowsVisibility();
	object_ptr<TWidget> doTakeInnerWidget();

	BoxContentDelegate *getDelegate() const {
		t_assert(_delegate != nullptr);
		return _delegate;
	}
	BoxContentDelegate *_delegate = nullptr;

	bool _noContentMargin = false;
	int _innerTopSkip = 0;
	object_ptr<Ui::ScrollArea> _scroll = { nullptr };
	object_ptr<Ui::WidgetFadeWrap<BoxLayerTitleShadow>> _topShadow = { nullptr };
	object_ptr<Ui::WidgetFadeWrap<BoxLayerTitleShadow>> _bottomShadow = { nullptr };

	object_ptr<QTimer> _draggingScrollTimer = { nullptr };
	int _draggingScrollDelta = 0;

};

class AbstractBox : public LayerWidget, public BoxContentDelegate, protected base::Subscriber {
public:
	AbstractBox(QWidget *parent, object_ptr<BoxContent> content);

	void parentResized() override;

	void setLayerType(bool layerType) override;
	void setTitle(const QString &title, const QString &additional) override;

	void clearButtons() override;
	QPointer<Ui::RoundButton> addButton(const QString &text, base::lambda<void()> &&clickCallback, const style::RoundButton &st) override;
	QPointer<Ui::RoundButton> addLeftButton(const QString &text, base::lambda<void()> &&clickCallback, const style::RoundButton &st) override;
	void updateButtonsPositions() override;

	void setDimensions(int newWidth, int maxHeight) override;

	void setNoContentMargin(bool noContentMargin) override {
		if (_noContentMargin != noContentMargin) {
			_noContentMargin = noContentMargin;
			updateSize();
		}
	}

	bool isBoxShown() const override {
		return !isHidden();
	}
	void closeBox() override {
		closeLayer();
	}

protected:
	void keyPressEvent(QKeyEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	void paintEvent(QPaintEvent *e) override;

	void doSetInnerFocus() override {
		_content->setInnerFocus();
	}
	void closeHook() override {
		_content->closeHook();
	}

private:
	void paintTitle(Painter &p, const QString &title, const QString &additional = QString());

	bool hasTitle() const;
	int titleHeight() const;
	int buttonsHeight() const;
	int buttonsTop() const;
	int contentTop() const;
	int countFullHeight() const;
	int countRealHeight() const;
	void updateSize();

	int _fullHeight = 0;

	bool _noContentMargin = false;
	int _maxContentHeight = 0;
	object_ptr<BoxContent> _content;

	QString _title;
	QString _additionalTitle;
	bool _layerType = false;

	std_::vector_of_moveable<object_ptr<Ui::RoundButton>> _buttons;
	object_ptr<Ui::RoundButton> _leftButton = { nullptr };

};

template <typename BoxType, typename ...Args>
inline object_ptr<BoxType> Box(Args&&... args) {
	auto parent = static_cast<QWidget*>(nullptr);
	return object_ptr<BoxType>(parent, std_::forward<Args>(args)...);
}

enum CreatingGroupType {
	CreatingGroupNone,
	CreatingGroupGroup,
	CreatingGroupChannel,
};
