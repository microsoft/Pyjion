#ifndef __COMPDATA_H__
#define __COMPDATA_H__

class Local {
public:
	int m_index;

	Local(int index = -1) {
		m_index = index;
	}

	bool is_valid() {
		return m_index != -1;
	}
};

class Label {
public:
	int m_index;

	Label(int index = -1) {
		m_index = index;
	}
};

enum BranchType {
	BranchAlways,
	BranchTrue,
	BranchFalse,
	BranchEqual,
	BranchNotEqual,
	BranchLeave,
};

class JittedCode {
public:
	virtual ~JittedCode() {
	}
	virtual void* get_code_addr() = 0;

};
#endif 