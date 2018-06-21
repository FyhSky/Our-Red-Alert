#include "CombatScene.h"
#include "ui\CocosGUI.h"
#include "Unit.h"

#define DEBUG
USING_NS_CC;
using namespace ui;

#define BOX_EDGE_WITDH_SMALL 10
#define BOX_EDGE_WITDH 40
#define SCROLL_LENGTH 10

void MouseRect::update(float f)
{
	clear();
	Node* parent = getParent();
	drawRect(touch_start, touch_end, Color4F(0, 1, 0, 1));
}
void CombatScene::DrawRectArea(Point p1, Point p2) {
	DrawNode* drawNode = DrawNode::create();
	this->addChild(drawNode);
	drawNode->drawRect(p1, p2, Color4F(0, 1, 0, 1));
}
//将框中的友方单位加入selectedd_ids
void CombatScene::getLayerUnit(Point p1, Point p2) {
	const auto&arrayNode = this->_combat_map->getChildren();
	for (auto&child : arrayNode) {
		Unit * pnode = static_cast<Unit *>(child);
		if (pnode && pnode->is_in(p1-delta, p2-delta) && (pnode->camp == unit_manager->player_id)) {
			unit_manager->selected_ids.push_back(pnode->id);
			log("%d", pnode->id);
		}
	}
}
Scene * CombatScene::createScene(chat_server * server_context_, chat_client * client_context_) {

	auto scene = Scene::create();
	auto layer = CombatScene::create(server_context_, client_context_);
	scene->addChild(layer);

	return scene;
}
CombatScene * CombatScene::create(chat_server * server_context_, chat_client * client_context_)
{

	CombatScene *combat_scene = new (std::nothrow) CombatScene();
	if (combat_scene && combat_scene->init(server_context_, client_context_))
	{
		combat_scene->autorelease();
		return combat_scene;
	}
	CC_SAFE_DELETE(combat_scene);

	return nullptr;

}

bool CombatScene::init(chat_server * server_context_, chat_client * client_context_) {
	if (!Layer::init()) {
		return false;
	}

	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();
	/*加载客户端和服务端*/
	server_side = server_context_;
	client_side = client_context_;
	/* 加载地图 */
	_combat_map = TMXTiledMap::create("map/BasicMap1.tmx");
	_combat_map->setAnchorPoint(Vec2(0, 0));
	addChild(_combat_map, 0);
	/* 加载格点地图 */
	_grid_map = GridMap::create(_combat_map);
	_grid_map->retain();
	/*加载矩形选框对象*/
	mouse_rect = MouseRect::create();
	mouse_rect->setVisible(false);
	addChild(mouse_rect, 15);
	/*加载金钱layer*/
	auto moneyDisplay = Sprite::create("MoneyDisplay.png");
	addChild(moneyDisplay);
	moneyDisplay->setScale(0.2);
	moneyDisplay->setPosition(visibleSize.width - 150, visibleSize.height - 30);
	money = Money::create();
	addChild(money, 40);
	money->setScale(2);
	money->setPosition(visibleSize.width - 130, visibleSize.height-30);
	money->schedule(schedule_selector(Money::update));
	/*加载电力条power*/
	auto powerSprite = Sprite::create("powerDisplay.png");
	addChild(powerSprite);
	powerSprite->setScale(0.2);
	powerSprite->setPosition(visibleSize.width - 150, visibleSize.height - 80);
	power = Power::create();
	addChild(power, 40);
	power->setPosition(visibleSize.width -190, visibleSize.height -95);
	PowerDisplay* powerDisplay = PowerDisplay::create();
	powerDisplay->setScale(2);
	powerDisplay->setPosition(visibleSize.width - 120, visibleSize.height - 80);
	addChild(powerDisplay, 41);
	power->powerDisplay = powerDisplay;
	power->updatePowerDisplay();

	msgs = new GameMessageSet;
	unit_manager = UnitManager::create();
	unit_manager->retain();
	unit_manager->setMessageSet(msgs);
	unit_manager->setTiledMap(_combat_map);
	unit_manager->setGridMap(_grid_map);
	unit_manager->setCombatScene(this);
	unit_manager->money = money;
	unit_manager->power = power;
	unit_manager->setSocketClient(client_side);
	unit_manager->setPlayerNum(client_side);
#ifdef DEBUG//测试
	auto farmer_sprite = Unit::create("MagentaSquare.png");
	farmer_sprite->setPosition(Vec2(visibleSize.width / 2 + 100, visibleSize.height / 2));
	this->_combat_map->addChild(farmer_sprite,10);
#endif
	/*刷新接受滚轮响应*/
	schedule(schedule_selector(CombatScene::update));
	
	/* 得到鼠标每一帧的位置 */
	auto mouse_event = EventListenerMouse::create();
	mouse_event->onMouseMove = [&](Event *event) {
		
		EventMouse* pem = static_cast<EventMouse*>(event);
		_cursor_position = Vec2(pem->getCursorX(), pem->getCursorY());
	};
	Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(mouse_event, 1);

	//得到初始化单位
	unit_manager->setPlayerID(client_side->camp());
	//unit_manager->setPlayerID(2);
	unit_manager->initializeUnitGroup();
	

	/*加载精灵监听器事件*/
	auto spriteListener = EventListenerTouchOneByOne::create();
	spriteListener->setSwallowTouches(true);
	spriteListener->onTouchBegan = [this](Touch* touch, Event* event) {
		mouse_rect->touch_start = touch->getLocation();
		auto target = static_cast<Unit*>(event->getCurrentTarget());
		Point locationInNode = target->convertToNodeSpace(touch->getLocation());
		Size s = target->getContentSize();
		Rect rect = Rect(0, 0, s.width, s.height);
		if (rect.containsPoint(locationInNode)) {
			unit_manager->selectPointUnits(target);
			return true;
		}
		return false;
	};
	spriteListener->onTouchEnded = [this](Touch* touch, Event* event) {
		mouse_rect->touch_end = touch->getLocation();
	};
	unit_manager->setSpriteTouchListener(spriteListener);
#ifdef DEBUG
	schedule(schedule_selector(CombatScene::update));
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(spriteListener, farmer_sprite);
#endif
	/*加载层监听器时间*/
	destListener = EventListenerTouchOneByOne::create();

	destListener->onTouchBegan = [this](Touch* touch, Event* event) {
		mouse_rect->touch_start = touch->getLocation();
		mouse_rect->schedule(schedule_selector(MouseRect::update));
		return true;
	};

	destListener->onTouchMoved = [this](Touch* touch, Event* event) {
		mouse_rect->touch_end = touch->getLocation();//更新最后接触的点
		mouse_rect->setVisible(true);
	};

	destListener->onTouchEnded = [this](Touch* touch, Event* event) {
		if (mouse_rect->isScheduled(schedule_selector(MouseRect::update)))
			mouse_rect->unschedule(schedule_selector(MouseRect::update));
		mouse_rect->touch_end = touch->getLocation();			
		mouse_rect->setVisible(false);
		//如果是框选而非点选则清空selecetd_ids
		if (Tri_Dsitance(mouse_rect->touch_start, mouse_rect->touch_end) > 8) {
			unit_manager->cancellClickedUnit();
		}
		//如果是点选，则执行selecctEmpty
		if (unit_manager->selected_ids.size()) {
			unit_manager->selectEmpty(mouse_rect->touch_end - delta);
		}
		//如果是框选，就将框中所有的己方单位添加到selected_ids并执行操作
		else {		
			if (mouse_rect->isScheduled(schedule_selector(MouseRect::update))) {
				mouse_rect->unschedule(schedule_selector(MouseRect::update));
			}
			getLayerUnit(mouse_rect->touch_start, mouse_rect->touch_end);
			unit_manager->getClickedUnit();
		}
	};
	Director::getInstance()->getEventDispatcher()
		->addEventListenerWithSceneGraphPriority(destListener, this->_combat_map);
	return true;
}

void CombatScene::focusOnBase(){
	auto visibleSize = Director::getInstance()->getVisibleSize();
	auto base_point = unit_manager->getBasePosition();
	Vec2 base_vec = base_point - visibleSize / 2;

	/* 如果以基地为中心的视野超出了TiledMap的大小 */
	if(_combat_map->getBoundingBox().size.height<base_vec.y+visibleSize.height)
		base_vec.y = _combat_map->getBoundingBox().size.height - visibleSize.height;
	if (_combat_map->getBoundingBox().size.width < base_vec.x + visibleSize.width)
		base_vec.x = _combat_map->getBoundingBox().size.width - visibleSize.width;
	if (base_vec.x < 0)
		base_vec.x = 0;
	if (base_vec.y < 0)
		base_vec.y = 0;

	_combat_map->setPosition(Point(0, 0) - base_vec);
}

void CombatScene::update(float f){
	unit_manager->updateUnitsState();
	scrollMap();
	delta = _combat_map->getPosition();
}
void CombatScene::scrollMap(){
	auto map_center = _combat_map->getPosition();
	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	int horizontal_state, vertical_state;
	horizontal_state = (origin.x + visibleSize.width - BOX_EDGE_WITDH_SMALL < _cursor_position.x)
		+ (origin.x + visibleSize.width - BOX_EDGE_WITDH < _cursor_position.x)
		- (origin.x + BOX_EDGE_WITDH_SMALL > _cursor_position.x)
		- (origin.x + BOX_EDGE_WITDH > _cursor_position.x);

	vertical_state = (origin.y + visibleSize.height - BOX_EDGE_WITDH_SMALL < _cursor_position.y)
		+ (origin.y + visibleSize.height - BOX_EDGE_WITDH < _cursor_position.y)
		- (origin.y + BOX_EDGE_WITDH_SMALL > _cursor_position.y)
		- (origin.y + BOX_EDGE_WITDH > _cursor_position.y);

	Vec2 scroll(0, 0);
	scroll += Vec2(-SCROLL_LENGTH, 0)*horizontal_state;
	scroll += Vec2(0, -SCROLL_LENGTH)*vertical_state;
	map_center += scroll;
	//move_amount -= scroll;
	if (_combat_map->getBoundingBox().containsPoint((-scroll) + Director::getInstance()->getVisibleSize())
		&& _combat_map->getBoundingBox().containsPoint(-scroll)) {
		_combat_map->setPosition(map_center);
	}
	

}

void Money::update(float f)
{
	if ((++timer % update_period == 0))
	{
		money += increase_amount;
		updateMoneyDisplay();
	}
}

bool Money::init()
{
	money = INITIAL_MONEY;
	char money_str[30];
	sprintf(money_str, "%d", money);
	return (money_str, "fonts/Money.fnt");
}

void Money::updateMoneyDisplay()
{
	char money_str[30];
	sprintf(money_str, "%d", money);
	setString(money_str);
}

bool Money::checkMoney(int price) const
{
	return (money >= price);
}

void Money::spendMoney(int cost)
{
	money -= cost;
	updateMoneyDisplay();
}

int Money::getIncreasingAmount() const
{
	return increase_amount;
}

void Money::setIncreasingAmount(int mount)
{
	increase_amount += mount;
}


void Power::addMax_power(int delta)
{
	max_power += delta;
}

void Power::spendPower(int power)
{
	used_power += power;
}

void Power::setCur_length()
{
	cur_length = (float)used_power / max_power * length;
}

bool Power::checkPower(int delta)
{
	return (used_power + delta <= max_power);
}

Color4F Power::getColor()
{
	if (used_power <= max_power / 3)
	{
		return green;
	}
	else if (used_power <= max_power / 3 * 2)
	{
		return yellow;
	}
	else
		return red;
}

void Power::updatePowerDisplay()
{
	clear();
	powerDisplay->updateDisplay(this);
	setCur_length();
	drawSolidRect(Point(0, 0), Point(length, width), blue);
	drawSolidRect(Point(0,0), Point(cur_length,width), getColor());
}

bool PowerDisplay::init()
{
	char power_str[30];
	sprintf(power_str, "UsedPower/MaxPower");
	return (power_str, "fonts/Power.fnt");
}

void PowerDisplay::updateDisplay(Power * power)
{
	char power_str[30];
	sprintf(power_str, "%d/%d", power->used_power,power->max_power);
	setString(power_str);
}
