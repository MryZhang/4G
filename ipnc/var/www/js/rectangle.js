//�ӿڷ���˵����
//1��������ʽ��Ϊjson��ʽ�Ķ���
//2������˫���ص�������
//3�����췽��  r : ���ʣ�x,y :���ꣻw,h : ��͸�   cw:canvas���  ch:canvas���
function Rectangle(r,cw,ch, x, y, w, h) {
    var CANVAS_WIDTH = cw;
    var CANVAS_HEIGHT = ch;

    /*
     *����
     */
    this.style = {
        fill : "none",
        "stroke-width" : "6",
        stroke : "red",
        "stroke-opacity" : "0.5"
    };
    this.title = "�������";
    this.dbClickHandler = null;
    
    this.itemSelectedStyle = {
        stroke : "red"
    };

    /*
     * ����
     */
    this.setStyle = function(style) {
        this.style = style;
        rect.attr(this.style);
    };
    //����˫���ص�����
    this.setDBClickHandler = function(value) {
        this.dbClickHandler = value;
    };
    //ѡ���Լ�
    this.itemSelect = function() {
        if (ancher)
            return;
        ancher = new RectAncher(r, rect);
        rect.attr(this.itemSelectedStyle);
    };
    this.itemUnSelect = function() {
        //����Ҫremoveê��
        if (ancher)
            ancher.removeAncher();
        ancher = null;
        rect.attr(this.style);
    };
    /**
     *���ؾ��ε� attr
     */
    this.getDrawInfo = function(){
        
        return rect.attr();
    };

    var that = this;
    //ʹ��thatָ�� Rectangle����
    var ancher = null;
    //δѡ��Ϊnul��ѡ�в�Ϊnull
    var doubleClickHandler = function(e) {
        if (that.dbClickHandler) {
            that.dbClickHandler();
        }
        //ѡ���Լ�
        that.itemSelect();
    };

    var onmove = function(dx, dy) {
        var rectAttr = {
            x : Math.min(Math.max(this.ox + dx, 0), CANVAS_WIDTH - rect.attr("width")),
            y : Math.min(Math.max(this.oy + dy, 0), CANVAS_HEIGHT - rect.attr("height"))
        };
        this.attr(rectAttr);
        if (ancher) {
            /*
             ancher.updateRectAncher(dx-(this.ax || 0),dy-(this.ay||0));
             this.ax = dx;
             this.ay = dy;
             */
            ancher.updateRectAncher(dx, dy);
        }
    }, onstart = function() {
        this.ox = this.attr("x");
        this.oy = this.attr("y");
        this.attr({
            stroke : "red",
            cursor : "move"
        });
        //this.ax = 0;
        //this.ay = 0;
        if (ancher) {
            ancher.setBaseXY();
        }
    }, onend = function() {
        if (ancher) {
            this.attr(that.itemSelectedStyle);
        } else {
            this.attr(that.style);
        }

    };

    //���캯������һ������
    var rect = r.rect(x, y, w, h).attr(this.style);
    //���캯��Ϊ��������϶��¼�
    rect.drag(onmove, onstart, onend);
    rect.dblclick(doubleClickHandler);
    //���캯��Ϊ�������˫���¼�

    /**
     * ê�㹹�캯�� 
     * @param {Object} r
     * @param {Object} rect
     */
    function RectAncher(r, rect) {
        var ancherW = 10, ancherH = 10;
        //�϶�������ÿ����
        this.updateRectAncher = function(dx, dy) {

            ancher_l_t.attr({
                x : Math.min(Math.max(ancher_l_t.baseX + dx, 0 - ancherW / 2), CANVAS_WIDTH - rect.attr("width") - ancherW / 2),
                y : Math.min(Math.max(ancher_l_t.baseY + dy, 0 - ancherH / 2), CANVAS_HEIGHT - rect.attr("height") - ancherH / 2)
            });
            ancher_r_t.attr({
                x : Math.min(Math.max(ancher_r_t.baseX + dx, 0 - ancherW / 2 + rect.attr("width")), CANVAS_WIDTH - ancherW / 2),
                y : Math.min(Math.max(ancher_r_t.baseY + dy, 0 - ancherH / 2), CANVAS_HEIGHT - rect.attr("height") - ancherH / 2)
            });
            ancher_l_b.attr({
                x : Math.min(Math.max(ancher_l_b.baseX + dx, 0 - ancherW / 2), CANVAS_WIDTH - rect.attr("width") - ancherW / 2),
                y : Math.min(Math.max(ancher_l_b.baseY + dy, rect.attr("height") - ancherH / 2), CANVAS_HEIGHT - ancherH / 2)
            });
            ancher_r_b.attr({
                x : Math.min(Math.max(ancher_r_b.baseX + dx, rect.attr("width") - ancherW / 2), CANVAS_WIDTH - ancherW / 2),
                y : Math.min(Math.max(ancher_r_b.baseY + dy, rect.attr("height") - ancherH / 2), CANVAS_HEIGHT - ancherH / 2)
            });

        };
        this.setBaseXY = function() {
            for (var i = 0; i < set.length; i++) {
                set[i].baseX = set[i].attr("x");
                set[i].baseY = set[i].attr("y");
            }
        };
        this.removeAncher = function() {
            set.remove();
        };

        //����ê��������Ϣ
        var attr_l_t = {
            x : rect.attr("x") - ancherW / 2,
            y : rect.attr("y") - ancherH / 2
        };
        var attr_r_t = {
            x : rect.attr("x") + rect.attr("width") - ancherW / 2,
            y : rect.attr("y") - ancherH / 2
        };
        var attr_l_b = {
            x : rect.attr("x") - ancherW / 2,
            y : rect.attr("y") + rect.attr("height") - ancherH / 2
        };
        var attr_r_b = {
            x : rect.attr("x") + rect.attr("width") - ancherW / 2,
            y : rect.attr("y") + rect.attr("height") - ancherH / 2
        };
        //����ê��
        var ancher_l_t = r.rect(attr_l_t.x, attr_l_t.y, ancherW, ancherH).attr({
            cursor : "nw-resize"
        });
        var ancher_r_t = r.rect(attr_r_t.x, attr_r_t.y, ancherW, ancherH).attr({
            cursor : "ne-resize"
        });
        var ancher_l_b = r.rect(attr_l_b.x, attr_l_b.y, ancherW, ancherH).attr({
            cursor : "sw-resize"
        });
        var ancher_r_b = r.rect(attr_r_b.x, attr_r_b.y, ancherW, ancherH).attr({
            cursor : "se-resize"
        });

        //ê�����set���򻯿���
        var set = r.set();
        set.push(ancher_l_t, ancher_r_t, ancher_l_b, ancher_r_b);
        set.attr({
            fill : "#0f0"
        });

        ancher_l_t.update = function(x, y) {
            var X = Math.min(Math.max(this.ox + x, ancherW / 2), ancher_r_t.attr("x") - ancherW);
            var Y = Math.min(Math.max(this.oy + y, ancherH / 2), ancher_l_b.attr("y") - ancherH);

            this.attr({
                x : X,
                y : Y
            });
            ancher_r_t.attr({
                x : ancher_r_t.attr("x"),
                y : Y
            });
            ancher_l_b.attr({
                x : X,
                y : ancher_l_b.attr("y")
            });
            updateRect();
        };
        ancher_r_t.update = function(x, y) {
            var X = Math.min(Math.max(this.ox + x, ancher_l_t.attr("x") + ancherW), CANVAS_WIDTH - ancherW / 2);
            var Y = Math.min(Math.max(this.oy + y, ancherH / 2), ancher_r_b.attr("y") - ancherH);
            this.attr({
                x : X,
                y : Y
            });
            ancher_l_t.attr({
                x : ancher_l_t.attr("x"),
                y : Y
            });
            ancher_r_b.attr({
                x : X,
                y : ancher_r_b.attr("y")
            });
            updateRect();

        };
        ancher_l_b.update = function(x, y) {
            var X = Math.min(Math.max(this.ox + x, ancherW / 2), ancher_r_t.attr("x") - ancherW);
            var Y = Math.min(Math.max(this.oy + y, ancher_l_t.attr("y") + ancherH), CANVAS_HEIGHT - ancherH / 2);
            this.attr({
                x : X,
                y : Y
            });
            ancher_l_t.attr({
                x : X,
                y : ancher_l_t.attr("y")
            });
            ancher_r_b.attr({
                x : ancher_r_b.attr("x"),
                y : Y
            });
            updateRect();
        };
        ancher_r_b.update = function(dx, dy) {
            var X = Math.min(Math.max(this.ox + dx, ancher_l_b.attr("x") + ancherW), CANVAS_WIDTH - ancherW / 2);
            var Y = Math.min(Math.max(this.oy + dy, ancher_r_t.attr("y") + ancherH), CANVAS_HEIGHT - ancherH / 2);
            this.attr({
                x : X,
                y : Y
            });

            ancher_r_t.attr({
                x : X,
                y : ancher_r_t.attr("y")
            });
            ancher_l_b.attr({
                x : ancher_l_b.attr("x"),
                y : Y
            });
            updateRect();
        };

        var updateRect = function() {
            var rectAttr = {
                x : ancher_l_t.attr("x") + ancherW / 2,
                y : ancher_l_t.attr("y") + ancherH / 2,
                width : ancher_r_t.attr("x") - ancher_l_t.attr("x"),
                height : ancher_l_b.attr("y") - ancher_l_t.attr("y")
            };

            rect.attr(rectAttr);
        };
        var move = function(dx, dy) {

            this.update(dx, dy);
        }, start = function() {
            this.ox = this.attr("x");
            this.oy = this.attr("y");
        }, up = function() {
        };
        set.drag(move, start, up);
    };
};