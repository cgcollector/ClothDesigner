#include "Graph.h"
#include "GraphPoint.h"
#include "AbstractGraphCurve.h"
#include "GraphLoop.h"
#include "tinyxml\tinyxml.h"
#include "cloth\definations.h"
namespace ldp
{
	Graph::Graph() : AbstractGraphObject()
	{
	
	}

	void Graph::clear()
	{
		m_keyPoints.clear();
		m_curves.clear();
		m_loops.clear();
		m_ptrMapAfterClone.clear();
	}

	Graph* Graph::clone()const
	{
		m_ptrMapAfterClone.clear();

		Graph* graph = new Graph();
		graph->setSelected(isSelected());

		for (auto iter : m_keyPoints)
		{
			std::shared_ptr<GraphPoint> kp(iter.second->clone());
			m_ptrMapAfterClone.insert(std::make_pair(iter.second.get(), kp.get()));
			graph->m_keyPoints.insert(std::make_pair(kp->getId(), kp));
		}
		for (auto iter : m_curves)
		{
			std::shared_ptr<AbstractGraphCurve> kp(iter.second->clone());
			m_ptrMapAfterClone.insert(std::make_pair(iter.second.get(), kp.get()));
			graph->m_curves.insert(std::make_pair(kp->getId(), kp));
		}
		for (auto iter : m_loops)
		{
			std::shared_ptr<GraphLoop> kp(iter.second->clone());
			m_ptrMapAfterClone.insert(std::make_pair(iter.second.get(), kp.get()));
			graph->m_loops.insert(std::make_pair(kp->getId(), kp));
		}

		for (auto iter : graph->m_keyPoints)
		{
			auto tmp = iter.second->edges();
			iter.second->edges().clear();
			for (auto e : tmp)
				iter.second->edges().insert((AbstractGraphCurve*)m_ptrMapAfterClone[e]);
		}
		for (auto iter : graph->m_curves)
		{
			iter.second->nextEdge() = (AbstractGraphCurve*)m_ptrMapAfterClone[iter.second->nextEdge()];
			for (int i = 0; i < iter.second->numKeyPoints(); i++)
				iter.second->keyPoint(i) = (GraphPoint*)m_ptrMapAfterClone[iter.second->keyPoint(i)];
		}
		for (auto iter : graph->m_loops)
		{
			iter.second->startEdge() = (AbstractGraphCurve*)m_ptrMapAfterClone[iter.second->startEdge()];
		}

		graph->m_bbox[0] = m_bbox[0];
		graph->m_bbox[1] = m_bbox[1];
		return graph;
	}

	TiXmlElement* Graph::toXML(TiXmlNode* parent)const
	{
		TiXmlElement* ele = AbstractGraphObject::toXML(parent);

		TiXmlElement* ele_kpts = new TiXmlElement("key-points");
		ele->LinkEndChild(ele_kpts);
		for (auto& p : m_keyPoints)
			p.second->toXML(ele_kpts);
		
		TiXmlElement* ele_cvs = new TiXmlElement("curves");
		ele->LinkEndChild(ele_cvs);
		for (auto& p : m_curves)
			p.second->toXML(ele_cvs);

		TiXmlElement* ele_loops = new TiXmlElement("loops");
		ele->LinkEndChild(ele_loops);
		for (auto& p : m_loops)
			p.second->toXML(ele_loops);

		return ele;
	}

	void Graph::fromXML(TiXmlElement* self)
	{
		clear();
		AbstractGraphObject::fromXML(self);

		for (auto groups = self->FirstChildElement(); groups; groups = groups->NextSiblingElement())
		{
			if (groups->Value() == std::string("key-points"))
			{
				for (auto child = groups->FirstChildElement(); child; child = child->NextSiblingElement())
				{
					GraphPointPtr kpt((GraphPoint*)AbstractGraphObject::create(child->Value()));
					kpt->fromXML(child);
					addKeyPoint(kpt, false);
				}
			} // end if key-pts
			if (groups->Value() == std::string("curves"))
			{
				for (auto child = groups->FirstChildElement(); child; child = child->NextSiblingElement())
				{
					AbstractGraphCurvePtr kpt((AbstractGraphCurve*)AbstractGraphObject::create(child->Value()));
					kpt->fromXML(child);
					addCurve(kpt);
				}
			} // end if curves
			if (groups->Value() == std::string("loops"))
			{
				for (auto child = groups->FirstChildElement(); child; child = child->NextSiblingElement())
				{
					GraphLoopPtr kpt((GraphLoop*)AbstractGraphObject::create(child->Value()));
					kpt->fromXML(child);
					addLoop(kpt);
				}
			} // end if loops
		} // end for groups
		updateBound();

		// auto make bounding loop
		std::vector<GraphLoop*> closedLoops;
		for (auto iter : m_loops)
		if (iter.second->isClosed())
			closedLoops.push_back(iter.second.get());
		if (closedLoops.size() == 1)
			closedLoops[0]->setBoundingLoop(true);
	}

	bool Graph::select(int idx, SelectOp op)
	{
		if (op == SelectEnd)
			return false;
		std::set<int> idxSet;
		idxSet.insert(idx);
		return select(idxSet, op);
	}

	bool Graph::select(const std::set<int>& idxSet, SelectOp op)
	{
		if (op == SelectEnd)
			return false;
		static std::vector<AbstractGraphObject*> objs;
		objs.clear();
		for (auto& o : m_keyPoints)
			objs.push_back(o.second.get());
		for (auto& o : m_curves)
			objs.push_back(o.second.get());
		for (auto& o : m_loops)
			objs.push_back(o.second.get());
		objs.push_back(this);
		bool changed = false;
		for (auto& obj : objs)
		{
			bool oldSel = obj->isSelected();
			switch (op)
			{
			case ldp::AbstractGraphObject::SelectThis:
				if (idxSet.find(obj->getId()) != idxSet.end())
					obj->setSelected(true);
				else
					obj->setSelected(false);
				break;
			case ldp::AbstractGraphObject::SelectUnion:
				if (idxSet.find(obj->getId()) != idxSet.end())
					obj->setSelected(true);
				break;
			case ldp::AbstractGraphObject::SelectUnionInverse:
				if (idxSet.find(obj->getId()) != idxSet.end())
					obj->setSelected(!obj->isSelected());
				break;
			case ldp::AbstractGraphObject::SelectAll:
				obj->setSelected(true);
				break;
			case ldp::AbstractGraphObject::SelectNone:
				obj->setSelected(false);
				break;
			case ldp::AbstractGraphObject::SelectInverse:
				obj->setSelected(!obj->isSelected());
				break;
			default:
				break;
			}
			bool newSel = obj->isSelected();
			if (oldSel != newSel)
				changed = true;
		} // end for obj
		return changed;
	}

	void Graph::highLight(int idx, int lastIdx)
	{
		auto cur = getObjByIdx(idx);
		if (cur)
			cur->setHighlighted(true);
		if (idx != lastIdx)
		{
			auto pre = getObjByIdx(lastIdx);
			if (pre)
				pre->setHighlighted(false);
		}
	}

	void Graph::updateBound(Float2& bmin, Float2& bmax)
	{
		m_bbox[0] = FLT_MAX;
		m_bbox[1] = -FLT_MAX;

		for (const auto& o : m_curves)
			o.second->unionBound(m_bbox[0], m_bbox[1]);

		for (int k = 0; k < bmin.size(); k++)
		{
			bmin[k] = std::min(bmin[k], m_bbox[0][k]);
			bmax[k] = std::max(bmax[k], m_bbox[1][k]);
		}
	}

	bool Graph::makeGraphValid(std::vector<std::shared_ptr<GraphsSewing>>& graphSewings)
	{
		// 1. check the bounding loop

		return true;
	}
	//////////////// topology operations: add units///////////////////////////////////////////
	GraphPoint* Graph::addKeyPoint(ldp::Float2 p, bool isEndPoint)
	{
		GraphPointPtr kp(new GraphPoint(p));
		return addKeyPoint(kp, isEndPoint);
	}

	GraphPoint* Graph::addKeyPoint(const std::shared_ptr<GraphPoint>& kp, bool isEndPoint)
	{
		auto iter = m_keyPoints.find(kp->getId());
		if (iter != m_keyPoints.end())
		{
			printf("warning: key point %d already existed!\n", kp->getId());
			return iter->second.get();
		}

		// a point exist that is close enough with p
		if (isEndPoint)
		{
			for (auto iter : m_keyPoints)
			{
				if ((iter.second->position() - kp->position()).length() < g_designParam.pointMergeDistThre)
					return iter.second.get();
			} // end for iter
		}

		m_keyPoints.insert(std::make_pair(kp->getId(), kp));
		return kp.get();
	}

	AbstractGraphCurve* Graph::addCurve(const std::vector<std::shared_ptr<GraphPoint>>& kpts)
	{
		std::vector<GraphPoint*> ptr;
		for (const auto& kp : kpts)
			ptr.push_back(addKeyPoint(kp, kp == kpts.front() || kp == kpts.back()));
		return addCurve(ptr);
	}

	AbstractGraphCurve* Graph::addCurve(const std::vector<GraphPoint*>& kpts)
	{
		AbstractGraphCurvePtr curve(AbstractGraphCurve::create(kpts));
		return addCurve(curve);
	}

	AbstractGraphCurve* Graph::addCurve(const std::shared_ptr<AbstractGraphCurve>& curve)
	{
		if (curve.get() == nullptr)
			return nullptr;

		// check curve id
		auto iter = m_curves.find(curve->getId());
		if (iter != m_curves.end())
		{
			printf("warning: addCurve: curve %d already existed!\n", curve->getId());
			return curve.get();
		}

		// check curve points
		for (auto iter : m_curves)
		{
			bool samePoints = true;
			if (iter.second->numKeyPoints() != curve->numKeyPoints())
				samePoints = false;
			for (int i = 0; i < iter.second->numKeyPoints(); i++)
			{
				if (iter.second->keyPoint(i) != curve->keyPoint(i)
					&& iter.second->keyPoint(i) != curve->keyPoint(curve->numKeyPoints() - 1 - i))
				{
					samePoints = false;
					break;
				}
			}
			if (samePoints)
				return iter.second.get();
		} // end for iter

		// add the curve
		for (int i = 0; i < curve->numKeyPoints(); i++)
		{
			auto kp = curve->keyPoint(i);
			if (m_keyPoints.find(kp->getId()) == m_keyPoints.end())
				throw std::exception("addCurve: keyPoints not exist!");
			kp->edges().insert(curve.get());
		} // end for i

		m_curves.insert(std::make_pair(curve->getId(), curve));
		return curve.get();
	}

	GraphLoop* Graph::addLoop(const std::vector<std::vector<GraphPointPtr>>& curves, bool isBoundingLoop)
	{
		std::vector<AbstractGraphCurve*> ptr;
		for (const auto& c : curves)
			ptr.push_back(addCurve(c));
		return addLoop(ptr, isBoundingLoop);
	}

	GraphLoop* Graph::addLoop(const std::vector<AbstractGraphCurvePtr>& curves, bool isBoundingLoop)
	{
		std::vector<AbstractGraphCurve*> ptr;
		for (const auto& c : curves)
			ptr.push_back(addCurve(c));
		return addLoop(ptr, isBoundingLoop);
	}

	GraphLoop* Graph::addLoop(const std::vector<AbstractGraphCurve*>& curves, bool isBoundingLoop)
	{
		if (curves.size() == 0)
			return nullptr;
		for (auto& c : curves)
		{
			if (m_curves.find(c->getId()) == m_curves.end())
				throw std::exception("addLoop: curve not exist!");
		}
		for (size_t i = 1; i < curves.size(); i++)
		{
			if (curves[i - 1]->getEndPoint() != curves[i]->getStartPoint())
				throw std::exception("addLoop: given curves not connected!");
			curves[i - 1]->nextEdge() = curves[i];
		}
		if (curves.back()->getEndPoint() == curves[0]->getStartPoint())
			curves.back()->nextEdge() = curves[0];
		std::shared_ptr<GraphLoop> loop(new GraphLoop);
		loop->startEdge() = curves[0];
		loop->setBoundingLoop(isBoundingLoop);
		return addLoop(loop);
	}

	GraphLoop* Graph::addLoop(const std::shared_ptr<GraphLoop>& loop)
	{
		if (loop->isBoundingLoop() && !loop->isClosed())
			throw std::exception("error: addLoop: bounding loop must be closed!");

		// check id
		auto iter = m_loops.find(loop->getId());
		if (iter != m_loops.end())
		{
			printf("warning: addLoop: loop %d already existed!\n", loop->getId());
			return loop.get();
		}

		// check overlap, we do not allow loop overlap
		std::hash_set<AbstractGraphCurve*> c1set;
		for (auto l : m_loops)
		{
			auto edge = l.second->startEdge();
			do
			{
				c1set.insert(edge);
				edge = edge->nextEdge();
			} while (edge && edge != l.second->startEdge());
		}
		auto edge = loop->startEdge();
		do
		{
			if (c1set.find(edge) != c1set.end())
			{
				printf("warning: over lapped edge %d for loop %d\n",
					edge->getId(), loop->getId());
				return nullptr;
			}
			edge = edge->nextEdge();
		} while (edge && edge != loop->startEdge());


		// check that there is only one bounding loop
		int nBounds = loop->isBoundingLoop();
		for (auto l : m_loops)
			nBounds += l.second->isBoundingLoop();
		if (nBounds > 1)
			throw std::exception("addLoop error: there must be exactly 1 bounding loop!");

		// insert loop
		m_loops.insert(std::make_pair(loop->getId(), loop));
		edge = loop->startEdge();
		do
		{
			edge->loop() = loop.get();
			edge = edge->nextEdge();
		} while (edge && edge != loop->startEdge());
		return loop.get();
	}

	GraphLoop* Graph::getBoundingLoop()
	{
		if (m_loops.size() == 0)
			return nullptr;
		int n = 0;
		GraphLoop* lp = nullptr;
		for (auto& iter : m_loops)
		if (iter.second->isBoundingLoop())
		{
			lp = iter.second.get();
			n++;
		}
		if (n != 1)
			throw std::exception("getBoundingLoop: there must be exactly 1 bounding loop!\n");
		return lp;
	}

	const GraphLoop* Graph::getBoundingLoop()const
	{
		if (m_loops.size() == 0)
			return nullptr;
		int n = 0;
		GraphLoop* lp = nullptr;
		for (auto& iter : m_loops)
		if (iter.second->isBoundingLoop())
		{
			lp = iter.second.get();
			n++;
		}
		if (n != 1)
			throw std::exception("const getBoundingLoop: there must be exactly 1 bounding loop!\n");
		return lp;
	}
	///////////////////// topology operations: remove units, return false if not removed//////////////
	bool Graph::remove(size_t id)
	{
		auto obj = getObjByIdx(id);
		if (obj == nullptr)
			return false;
		if (obj->isCurve())
			return removeCurve((AbstractGraphCurve*)obj);
		if (obj->getType() == TypeGraphPoint)
			return removeKeyPoints((GraphPoint*)obj);
		if (obj->getType() == TypeGraphLoop)
			return removeLoop((GraphLoop*)obj);
		return false;
	}

	bool Graph::removeKeyPoints(const GraphPoint* kp)
	{
		if (kp == nullptr)
			return false;
		auto iter = m_keyPoints.find(kp->getId());
		if (iter == m_keyPoints.end())
			return false;

		// remove related curves
		for (auto & c : kp->edges())
			removeCurve(c);

		// remove self
		iter->second->edges().clear();
		m_keyPoints.erase(iter);
		return true;
	}

	bool Graph::removeCurve(const AbstractGraphCurve* curve)
	{
		if (curve == nullptr)
			return false;

		auto iter = m_curves.find(curve->getId());
		if (iter == m_curves.end())
			return false;

		// check the loop consistency
		assert(iter->second->loop());
		auto prevEdge = iter->second->loop()->startEdge();
		do
		{
			if (prevEdge->nextEdge() == iter->second.get())
				break;
			prevEdge = prevEdge->nextEdge();
		} while (prevEdge && prevEdge != iter->second->loop()->startEdge());
		auto nextEdge = iter->second->nextEdge();

		// only one edge, remove the empty loop
		if (prevEdge == nullptr && nextEdge == nullptr)
			m_loops.erase(m_loops.find(iter->second->loop()->getId()));
		// start edge removed
		else if (prevEdge == nullptr && nextEdge)
			iter->second->loop()->startEdge() = nextEdge;
		// end edge removed, no change
		else if (prevEdge && nextEdge == nullptr)
			prevEdge->nextEdge() = nullptr;
		// middle edge removed, split needed
		else
		{
			prevEdge->nextEdge() = nullptr;
			std::shared_ptr<GraphLoop> loop(new GraphLoop);
			loop->startEdge() = nextEdge;
			addLoop(loop);
		}

		// remove its point
		for (int i = 0; i < iter->second->numKeyPoints(); i++)
		{
			GraphPoint* p = iter->second->keyPoint(i);
			if (p->edges().size() == 1 && *p->edges().begin() == iter->second.get())
				m_keyPoints.erase(m_keyPoints.find(p->getId()));
		} // end for i

		// remove self
		m_curves.erase(iter);
		return true;
	}

	bool Graph::removeLoop(const GraphLoop* loop)
	{
		if (loop == nullptr)
			return false;
		auto iter = m_loops.find(loop->getId());
		if (iter == m_loops.end())
			return false;

		auto edge = iter->second->startEdge();
		std::hash_set<AbstractGraphCurve*> tmp;
		do
		{
			edge->loop() = nullptr;
			tmp.insert(edge);
			edge = edge->nextEdge();
		} while (edge && edge != iter->second->startEdge());

		// by removing all edges, the loop itself is removed
		for (auto e : tmp)
			removeCurve(e);

		return true;
	}


	/////////// split a curve into two, return the inserted point and the newCurve generated/////////////
	GraphPoint* Graph::splitEdgeMakePoint(AbstractGraphCurve* curveToSplit, 
		float splitPos, AbstractGraphCurve*& newCurve)
	{
		splitPos = std::min(1.f, std::max(0.f, splitPos));

		return nullptr; // not implemented
	}


	/////////// two curves can be merged into one iff they share a common point///////////////////
	bool Graph::mergeCurve(AbstractGraphCurve* curve1, AbstractGraphCurve* curve2)
	{
		return nullptr; // not implmented
	}

}