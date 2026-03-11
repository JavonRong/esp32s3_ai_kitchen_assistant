# 此项目的一些笔记


## LVGL的UI学习 C++的继承、重写和多态
Dispaly->子类LcdDisplay
`virtual void SetupUI() override;` //重写


主容器container_, lv_obj_create(screen) 
layer1：top_bar + content
layer2：status bar

## static_cast 的基本语法
`static_cast<new_type>(expression)`  显式类型转换 
lvgl中 `auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);`  是向下类型转换

## LVGL的一些函数、参数等
`LV_HOR_RES`实际屏幕水平大小     
`LV_VER_RES`实际屏幕垂直大小   
`LV_SIZE_CONTENT`按照内容的大小设定大小

## C++构造函数以及成员初始化列表
    class DHT11Controller {
        private:
            gpio_num_t gpio_num_
            ...

        public:
            DHT11Controller(gpio_num_t gpio_num) : gpio_num_(gpio_num) {
                // 构造函数体
            }
    }
//其中DHT11Controller里的DHT11Controller(gpio_num_t gpio_num)就是一个构造函数，需要传入一个gpio_num_t类型的参数
: gpio_num_(gpio_num)这个的意思是传入的参数用来初始化私有成员变量 就是将传入的参数gpio_num赋值给gpio_num_

## 小智LVGL锁机制，用C++的类构造函数去加锁，用析构函数去解锁

